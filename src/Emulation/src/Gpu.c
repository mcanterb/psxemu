#include "Gpu.h"
#include "Clock.h"
#include "Dma.h"
#include "System.h"
#include <immintrin.h>
#include <string.h>

ASSUME_NONNULL_BEGIN

#define kCommandBufferSize 16

static const size_t kVramSize = 1024 * 512;
static const uint32_t kGpuClockRate = 53690000;
static const uint32_t kCyclesPerScanline = 3413;
static const uint32_t kCyclesPerFrame = 525 * kCyclesPerScanline;
static const uint32_t kCyclesPerDisplay = 480 * kCyclesPerScanline;
static const uint32_t kCyclesPerVBlank = kCyclesPerFrame - kCyclesPerDisplay;
static const int8_t kDitherTable[] = {-4, 0, -3, 1, 2, -1, 3, -1, -3, 1, -4, 0, 3, -1, 2, -2};

static GpuControlCommandHandler kControlCommandTable[64] = {
    GpuReset,
    GpuClearCommandBuffer,
    GpuResetIrq,
    GpuDisplayEnable,
    GpuSetDmaMode,
    GpuSetDisplayStart,
    GpuSetHorizontalDisplayRange,
    GpuSetVerticalDisplayRange,
    GpuSetDisplayMode,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuGetGpuInfo,
    GpuGetGpuInfo,
    GpuGetGpuInfo,
    GpuGetGpuInfo,
    GpuGetGpuInfo,
    GpuGetGpuInfo,
    GpuGetGpuInfo,
    GpuGetGpuInfo,
    GpuGetGpuInfo,
    GpuGetGpuInfo,
    GpuGetGpuInfo,
    GpuGetGpuInfo,
    GpuGetGpuInfo,
    GpuGetGpuInfo,
    GpuGetGpuInfo,
    GpuGetGpuInfo,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
    GpuInvalidControlCommand,
};

static uint32_t kColorRampLookup[32] = {0,   16,  32,  48,  64,  80,  96,  112, 128, 133, 139, 144, 150, 155, 161, 166,
                                        172, 177, 183, 188, 194, 199, 205, 210, 216, 221, 227, 232, 238, 243, 249, 255};

static void FastFill(GpuScreen screen, uint32_t value) {
  uint32_t *dest = screen.pixels;
  size_t size = screen.width * screen.height;
  size_t untilAlignment = (((size_t)dest) & 0x1F) >> 2;
  size_t streamEnd = (size - untilAlignment) & 0xFFFFFFFFFFFFFFF8;
  size_t i;
  for (i = 0; i < untilAlignment; i++) {
    dest[i] = value;
  }
  __m256i target = _mm256_set_epi32(value, value, value, value, value, value, value, value);
  while (i < streamEnd) {
    _mm256_store_si256((__m256i *)&dest[i], target);
    i += 8;
  }
  for (; i < size; i++) {
    dest[i] = value;
  }
}

typedef struct __BarycentricPoint {
  double p1;
  double p2;
  double p3;
} BarycentricPoint;

typedef union packed __GpuPackedVertex {
  uint32_t value;
  struct packed {
    uint16_t x;
    uint16_t y;
  } coords;
} GpuPackedVertex;

static inline GpuPackedVertex NewPackedVertex(uint32_t value) {
  GpuPackedVertex vert;
  vert.value = value;
  return vert;
}

typedef union packed __GpuPackedColor {
  uint32_t value;
  struct packed {
    uint32_t b : 8;
    uint32_t g : 8;
    uint32_t r : 8;
  } colors;
} GpuPackedColor;

static inline GpuPackedColor NewPackedColor(uint32_t value) {
  GpuPackedColor color;
  color.value = value;
  return color;
}

static double GetBarycentricScalar(GpuPackedVertex v1, GpuPackedVertex v2, GpuPackedVertex v3) {
  double x1 = v1.coords.x;
  double x2 = v2.coords.x;
  double x3 = v3.coords.x;
  double y1 = v1.coords.y;
  double y2 = v2.coords.y;
  double y3 = v3.coords.y;
  return ((y2 - y3) * (x1 - x3)) + ((x3 - x2) * (y1 - y3));
}

static BarycentricPoint GetBarycentricPoint(GpuPackedVertex v1, GpuPackedVertex v2, GpuPackedVertex v3, uint16_t x,
                                            uint16_t y, double scalar) {
  double x1 = v1.coords.x;
  double x2 = v2.coords.x;
  double x3 = v3.coords.x;
  double y1 = v1.coords.y;
  double y2 = v2.coords.y;
  double y3 = v3.coords.y;
  BarycentricPoint point;
  point.p1 = (((y2 - y3) * (x - x3)) + ((x3 - x2) * (y - y3))) / scalar;
  point.p2 = (((y3 - y1) * (x - x3)) + ((x1 - x3) * (y - y3))) / scalar;
  point.p3 = 1 - point.p1 - point.p2;
  return point;
}

static inline bool IsValidPoint(BarycentricPoint point) {
  return point.p1 >= 0.0 && point.p1 <= 1.0 && point.p2 >= 0.0 && point.p2 <= 1.0 && point.p3 >= 0.0 && point.p3 <= 1.0;
}

static inline uint32_t GouradShadedColor(BarycentricPoint v1, GpuPackedColor c1, GpuPackedColor c2, GpuPackedColor c3) {
  GpuPackedColor result;
  result.colors.b = v1.p1 * c1.colors.b + v1.p2 * c2.colors.b + v1.p3 * c3.colors.b;
  result.colors.g = v1.p1 * c1.colors.g + v1.p2 * c2.colors.g + v1.p3 * c3.colors.g;
  result.colors.r = v1.p1 * c1.colors.r + v1.p2 * c2.colors.r + v1.p3 * c3.colors.r;
  return result.value;
}

typedef struct __GpuContinuation {
  uint32_t cyclesSinceLastFrame;
  bool oddFrame;
  GpuCommandImpl runningCommand;
  uint32_t requiredPackets;
  bool isRunning;
  uint32_t cyclesToRun;
  bool writeToVram;
  uint16_t writeToVramX;
  uint16_t writeToVramY;
  size_t writeToVramIndex;
  uint16_t writeToVramWidth;
  uint16_t writeToVramHeight;
} GpuContinuation;

typedef union __GpuStatus {
  uint32_t value;
  struct packed __GpuStatusParsed {
    uint32_t texx : 4;                 // x = texx * 64
    uint32_t texy : 1;                 // 0=0; 1=256
    uint32_t semitransparencyMode : 2; // Semi transparent state
    uint32_t texPageColorMode : 2;     // 00=4bit CLUT; 01=8-Bit CLUT; 10=15bit
    uint32_t dither : 1;
    uint32_t allowDrawToDisplayArea : 1; // Draw to display area allowed = 1
    uint32_t maskSet : 1;
    uint32_t maskEnable : 1;
    uint32_t unknown : 1;
    uint32_t reverse : 1;
    uint32_t texDisable : 1;
    uint32_t width1 : 1;
    uint32_t width0 : 2;
    uint32_t height : 1;
    uint32_t pal : 1;
    uint32_t isrgb24 : 1;
    uint32_t isinter : 1;
    uint32_t displayDisabled : 1;
    uint32_t interrupt : 1;
    uint32_t dmaReq : 1;
    uint32_t commandReady : 1;
    uint32_t cpuReadReady : 1;
    uint32_t dmaReady : 1;
    uint32_t dma : 2;
    uint32_t lcf : 1; // 0 when drawing even lines in interlace mode
  } parsed;
} GpuStatus;

struct __Gpu {
  System *sys;
  ClockDeviceHandle clockHandle;
  GpuPacket packetBuffer[kCommandBufferSize];
  size_t bufferSize;
  size_t bufferStart;
  GpuStatus status;
  uint32_t commandResponse;

  GpuContinuation continuation;
  uint32_t screenWidth;
  uint32_t screenHeight;
  uint16_t displayStartX;
  uint16_t displayStartY;
  uint16_t displayRangeX1;
  uint16_t displayRangeX2;
  uint16_t displayRangeY1;
  uint16_t displayRangeY2;
  uint16_t drawingAreaLeft;
  uint16_t drawingAreaRight;
  uint16_t drawingAreaTop;
  uint16_t drawingAreaBottom;
  uint16_t drawingAreaOffsetX;
  uint16_t drawingAreaOffsetY;
  uint8_t texWindowMaskX;
  uint8_t texWindowMaskY;
  uint8_t texWindowOffsetX;
  uint8_t texWindowOffsetY;
  uint16_t vram[kVramSize];
};

static uint8_t Clamp(int16_t value) {
  if (value > 255) {
    return 255;
  }
  if (value < 0) {
    return 0;
  }
  return value;
}

static uint32_t Dither(uint16_t x, uint16_t y, uint32_t color) {
  GpuPackedColor c = NewPackedColor(color);
  int8_t offset = kDitherTable[(x & 3) + ((y & 3) << 2)];
  int16_t b = c.colors.b;
  int16_t g = c.colors.g;
  int16_t r = c.colors.r;
  c.colors.b = Clamp(b + offset);
  c.colors.g = Clamp(g + offset);
  c.colors.r = Clamp(r + offset);
  return c.value;
}

static void GpuSetPixel(Gpu *gpu, uint16_t x, uint16_t y, uint32_t color, bool dither) {
  x = gpu->drawingAreaOffsetX + x;
  y = gpu->drawingAreaOffsetY + y;
  if (x > gpu->drawingAreaRight || x < gpu->drawingAreaLeft || y > gpu->drawingAreaBottom || y < gpu->drawingAreaTop) {
    return;
  }
  if (gpu->status.parsed.maskEnable && (gpu->vram[x + (y * 1024)] & 0x8000)) {
    return;
  }
  if (dither) {
    color = Dither(x, y, color);
  }
  uint16_t screenPixel = ((color & 0xF8) >> 3);
  screenPixel |= ((color & 0xF800) >> 6);
  screenPixel |= ((color & 0xF80000) >> 9);
  screenPixel |= (gpu->status.parsed.maskSet << 15);
  gpu->vram[x + (y * 1024)] = screenPixel;
}

static void GpuMonochromeTriangle(Gpu *gpu, GpuPackedVertex v1, GpuPackedVertex v2, GpuPackedVertex v3,
                                  uint32_t color) {
  uint16_t left = fmin(v1.coords.x, fmin(v2.coords.x, v3.coords.x));
  uint16_t right = fmax(v1.coords.x, fmax(v2.coords.x, v3.coords.x));
  uint16_t top = fmin(v1.coords.y, fmin(v2.coords.y, v3.coords.y));
  uint16_t bottom = fmax(v1.coords.y, fmax(v2.coords.y, v3.coords.y));
  double scalar = GetBarycentricScalar(v1, v2, v3);
  uint16_t x, y;
  for (y = top; y < bottom; y++) {
    for (x = left; x < right; x++) {
      BarycentricPoint point = GetBarycentricPoint(v1, v2, v3, x, y, scalar);
      if (IsValidPoint(point)) {
        GpuSetPixel(gpu, x, y, color, false);
      }
    }
  }
}

static void GpuShadedTriangle(Gpu *gpu, GpuPackedVertex v1, GpuPackedVertex v2, GpuPackedVertex v3, GpuPackedColor c1,
                              GpuPackedColor c2, GpuPackedColor c3) {
  uint16_t left = fmin(v1.coords.x, fmin(v2.coords.x, v3.coords.x));
  uint16_t right = fmax(v1.coords.x, fmax(v2.coords.x, v3.coords.x));
  uint16_t top = fmin(v1.coords.y, fmin(v2.coords.y, v3.coords.y));
  uint16_t bottom = fmax(v1.coords.y, fmax(v2.coords.y, v3.coords.y));
  double scalar = GetBarycentricScalar(v1, v2, v3);
  uint16_t x, y;
  bool dither = gpu->status.parsed.dither;
  for (y = top; y < bottom; y++) {
    for (x = left; x < right; x++) {
      BarycentricPoint point = GetBarycentricPoint(v1, v2, v3, x, y, scalar);
      if (IsValidPoint(point)) {
        GpuSetPixel(gpu, x, y, GouradShadedColor(point, c1, c2, c3), dither);
      }
    }
  }
}

static void GpuBlit(Gpu *gpu, GpuScreen screen) {
  if (screen.width != gpu->screenWidth || screen.height != gpu->screenHeight) {
    PCF_PANIC("Screen size mismatch!");
  }
  size_t x, y, vramY;
  size_t vramOffset, screenOffset;
  int yIncrement = gpu->status.parsed.isinter + 1;
  y = gpu->status.parsed.isinter ? gpu->continuation.oddFrame : 0;
  vramY = 0;
  for (; y < gpu->screenHeight; y += yIncrement) {
    vramOffset = gpu->displayStartX + ((((size_t)gpu->displayStartY + vramY) * 1024));
    screenOffset = y * screen.width;
    for (x = 0; x < gpu->screenWidth; x++) {
      uint16_t pixel = gpu->vram[vramOffset + x];
      uint32_t screenPixel = (kColorRampLookup[pixel & 0x1F] << 16);
      screenPixel |= (kColorRampLookup[(pixel & 0x3E0) >> 5] << 8);
      screenPixel |= (kColorRampLookup[(pixel & 0x7C00) >> 10]);
      screen.pixels[screenOffset + x] = screenPixel;
    }
    vramY += yIncrement;
  }
}

static bool GpuBufferEmpty(Gpu *gpu) { return gpu->bufferSize == 0; }

static void GpuAddPacketToBuffer(Gpu *gpu, GpuPacket packet) {
  if (gpu->bufferSize == kCommandBufferSize) {
    gpu->packetBuffer[gpu->bufferStart] = packet;
    gpu->bufferStart = (gpu->bufferStart + 1) & 0xF;
  } else {
    gpu->packetBuffer[(gpu->bufferStart + gpu->bufferSize) & 0xF] = packet;
    gpu->bufferSize++;
  }
}

static GpuPacket GpuPeekPacket(Gpu *gpu) {
  if (gpu->bufferSize == 0) {
    return 0;
  }
  return gpu->packetBuffer[gpu->bufferStart];
}

static GpuPacket GpuPeekPacketAt(Gpu *gpu, size_t index) {
  if (gpu->bufferSize <= index) {
    return 0;
  }
  return gpu->packetBuffer[(gpu->bufferStart + index) & 0xF];
}

static GpuPacket GpuGetPacket(Gpu *gpu) {
  if (gpu->bufferSize == 0) {
    return 0;
  }
  GpuPacket result = gpu->packetBuffer[gpu->bufferStart];
  gpu->bufferStart = (gpu->bufferStart + 1) & 0xF;
  gpu->bufferSize--;
  return result;
}

static inline BusDevice GpuBusDevice(Gpu *gpu) {
  BusDevice device = {.context = gpu,
                      .cpuCycles = 0,
                      .read32 = (Read32)GpuRead32,
                      .read16 = (Read16)GpuRead16,
                      .read8 = (Read8)GpuRead8,
                      .write32 = (Write32)GpuWrite32,
                      .write16 = (Write16)GpuWrite16,
                      .write8 = (Write8)GpuWrite8};
  return device;
}

uint32_t GpuDmaChannelRead32(void *context, Address address) {
  Gpu *gpu = (Gpu *)context;
  return GpuGetCommandResponse(gpu);
}

void GpuDmaChannelWrite32(void *context, Address address, uint32_t value) {
  Gpu *gpu = (Gpu *)context;
  GpuSendCommand(gpu, value);
}

bool GpuDmaChannelIsReady(void *context) {
  Gpu *gpu = (Gpu *)context;
  return gpu->status.parsed.dmaReady;
}

Gpu *GpuNew(System *sys, Bus *bus) {
  Gpu *gpu = (Gpu *)SystemArenaAllocate(sys, sizeof(*gpu));
  gpu->screenWidth = 640;
  gpu->screenHeight = 480;
  gpu->sys = sys;
  gpu->status.value = 0x14802000;
  gpu->status.parsed.commandReady = 1;
  gpu->status.parsed.dmaReady = 1;
  gpu->status.parsed.cpuReadReady = 1;
  BusDevice device = GpuBusDevice(gpu);
  PCFResultOrPanic(BusRegisterDevice(bus, &device, NewAddressRange(0x1F801810, 0x1F801818, kMainSegments)));
  ClockDevice clockDevice = NewClockDevice(gpu, (UpdateHandler)GpuRun, kGpuClockRate);
  gpu->clockHandle = ClockAddDevice(SystemClock(sys), &clockDevice);
  ClockDeviceSetDefaultUpdateFrequency(gpu->clockHandle, kCyclesPerScanline);
  DmaChannelPort *port = DmaGetChannel(SystemDma(sys), DmaChannelGpu);
  DmaChannelSetClocksPerWord(port, 1);
  DmaChannelSetHandlers(port, gpu, GpuDmaChannelWrite32, GpuDmaChannelRead32, GpuDmaChannelIsReady);
  return gpu;
}

uint32_t GpuGetStatus(Gpu *gpu) { return gpu->status.value; }

uint32_t GpuGetCommandResponse(Gpu *gpu) { return gpu->commandResponse; }

static void GpuFinishCommand(Gpu *gpu) {
  gpu->continuation.runningCommand(gpu);
  gpu->continuation.isRunning = false;
  gpu->continuation.runningCommand = NULL;
  int i;
  for (i = 0; i < gpu->continuation.requiredPackets; i++) {
    GpuGetPacket(gpu);
  }
  gpu->continuation.requiredPackets = 0;
  gpu->continuation.cyclesToRun = 0;
  gpu->status.parsed.dmaReady = 1;
  gpu->status.parsed.commandReady = 1;
}

static void GpuBeginCommand(Gpu *gpu) {
  gpu->status.parsed.dmaReady = 0;
  gpu->status.parsed.commandReady = 0;
  gpu->continuation.isRunning = true;
}

static void GpuCommandDispatch(Gpu *gpu, GpuPacket packet) {
  uint32_t command = (packet & 0xFF000000);
  switch (command) {
  case 0x28000000:
    GpuRenderMonochromeQuad(gpu, packet);
    break;
  case 0xA0000000:
    GpuCopyRectCpuVram(gpu, packet);
    break;
  case 0xC0000000:
    GpuCopyRectVramCpu(gpu, packet);
    break;
  case 0x80000000:
    GpuCopyRectVramVram(gpu, packet);
    break;
  case 0x30000000:
    // GpuGetPacket(gpu);
    GpuRenderShadedTri(gpu, packet);
    break;
  case 0x38000000:
    GpuRenderShadedQuad(gpu, packet);
    break;
  default:
    GpuGetPacket(gpu);
  }
}

static void GpuProcessBuffer(Gpu *gpu, uint32_t cycles) {
  if (gpu->continuation.isRunning) {
    if (gpu->continuation.cyclesToRun > 0) {
      if (gpu->continuation.cyclesToRun > cycles) {
        gpu->continuation.cyclesToRun -= cycles;
      } else {
        uint32_t cyclesToRun = gpu->continuation.cyclesToRun;
        GpuFinishCommand(gpu);
        GpuProcessBuffer(gpu, cycles - cyclesToRun);
        return;
      }
    }
  }
  if (gpu->bufferSize == 0) {
    return;
  }
  if (gpu->continuation.requiredPackets == 0) {
    GpuPacket packet = GpuPeekPacket(gpu);
    GpuCommandDispatch(gpu, packet);
    return;
  }
  if (gpu->bufferSize >= gpu->continuation.requiredPackets) {
    GpuBeginCommand(gpu);
    ClockDeviceRequestUpdate(gpu->clockHandle, gpu->continuation.cyclesToRun);
  }
}

static void GpuWriteToVram(Gpu *gpu, GpuPacket packet) {
  size_t max = gpu->continuation.writeToVramHeight * gpu->continuation.writeToVramWidth;
  GpuPackedVertex values = NewPackedVertex(packet);
  size_t indexX = gpu->continuation.writeToVramIndex % gpu->continuation.writeToVramWidth;
  size_t indexY = gpu->continuation.writeToVramIndex / gpu->continuation.writeToVramWidth;
  size_t x = indexX + gpu->continuation.writeToVramX;
  size_t y = indexY + gpu->continuation.writeToVramY;
  gpu->vram[x + (y * 1024)] = values.coords.x;
  gpu->continuation.writeToVramIndex++;
  indexX = gpu->continuation.writeToVramIndex % gpu->continuation.writeToVramWidth;
  indexY = gpu->continuation.writeToVramIndex / gpu->continuation.writeToVramWidth;
  x = indexX + gpu->continuation.writeToVramX;
  y = indexY + gpu->continuation.writeToVramY;
  gpu->vram[x + (y * 1024)] = values.coords.y;
  gpu->continuation.writeToVramIndex++;
  if (gpu->continuation.writeToVramIndex >= max) {
    gpu->continuation.writeToVram = false;
  }
}

void GpuSendCommand(Gpu *gpu, GpuPacket packet) {
  if (gpu->continuation.writeToVram) {
    GpuWriteToVram(gpu, packet);
    return;
  }
  switch (packet & 0xFF000000) {
  case 0xE1000000:
    GpuSetDrawMode(gpu, packet);
    return;
  case 0xE2000000:
    GpuSetTextureWindow(gpu, packet);
    return;
  case 0xE3000000:
  case 0xE4000000:
    GpuSetDrawingArea(gpu, packet);
    return;
  case 0xE5000000:
    GpuSetDrawingOffset(gpu, packet);
    return;
  case 0xE6000000:
    GpuSetMaskBit(gpu, packet);
    return;
  }
  GpuAddPacketToBuffer(gpu, packet);
  GpuProcessBuffer(gpu, 0);
}
void GpuSendControl(Gpu *gpu, GpuPacket packet) {
  GpuCommand command = GpuPacketToCommand(packet);
  kControlCommandTable[command.parsed.command & 0x3F](gpu, command);
}

void GpuRun(Gpu *gpu, uint32_t cycles) {
  gpu->continuation.cyclesSinceLastFrame += cycles;
  if (gpu->continuation.cyclesSinceLastFrame >= kCyclesPerDisplay) {
    gpu->continuation.cyclesSinceLastFrame -= kCyclesPerDisplay;
    gpu->continuation.oddFrame = !gpu->continuation.oddFrame;
  }
  uint32_t line = gpu->continuation.cyclesSinceLastFrame / kCyclesPerScanline;
  if (gpu->status.parsed.isinter) {
    gpu->status.parsed.lcf = gpu->continuation.oddFrame;
  } else {
    gpu->status.parsed.lcf = line & 0x01;
  }
  if (gpu->bufferSize == 0) {
    return;
  }
  GpuProcessBuffer(gpu, cycles);
}

void GpuUpdateScreen(Gpu *gpu, GpuScreen screen) {
  if (gpu->status.parsed.displayDisabled) {
    FastFill(screen, 0x0018499E);
    return;
  }
  GpuBlit(gpu, screen);
}

uint32_t GpuScreenWidth(Gpu *gpu) { return gpu->screenWidth; }

uint32_t GpuScreenHeight(Gpu *gpu) { return gpu->screenHeight; }

void GpuInvalidControlCommand(Gpu *gpu, GpuCommand command) {
  PCF_PANIC("Received invalid control command 0x%08x", command.value);
}

void GpuReset(Gpu *gpu, GpuCommand command) {
  gpu->status.value = 0x14802000;
  gpu->status.parsed.commandReady = 1;
  gpu->status.parsed.dmaReady = 1;
  gpu->status.parsed.cpuReadReady = 1;
  gpu->commandResponse = 0;
  GpuClearCommandBuffer(gpu, GpuPacketToCommand(0x01000000));
}

void GpuClearCommandBuffer(Gpu *gpu, GpuCommand command) {
  gpu->bufferSize = 0;
  gpu->continuation.requiredPackets = 0;
  gpu->continuation.cyclesToRun = 0;
  gpu->continuation.runningCommand = NULL;
}

void GpuResetIrq(Gpu *gpu, GpuCommand command) { PCFDEBUG("Gpu -> Reset IRQ"); }

void GpuDisplayEnable(Gpu *gpu, GpuCommand command) { // I need this on the next line
  gpu->status.parsed.displayDisabled = command.parsed.parameters;
}

void GpuSetDmaMode(Gpu *gpu, GpuCommand command) { gpu->status.parsed.dma = command.parsed.parameters; }

void GpuSetDisplayStart(Gpu *gpu, GpuCommand command) {
  gpu->displayStartX = command.value & 0x000003FF;
  gpu->displayStartY = (command.value & 0x0007FC00) >> 10;
}

void GpuSetHorizontalDisplayRange(Gpu *gpu, GpuCommand command) {}

void GpuSetVerticalDisplayRange(Gpu *gpu, GpuCommand command) {}

void GpuSetDisplayMode(Gpu *gpu, GpuCommand command) {
  gpu->status.parsed.width0 = command.value & 0x00000003;
  gpu->status.parsed.height = (command.value & 0x00000004) >> 2;
  gpu->status.parsed.pal = (command.value & 0x00000008) >> 3;
  gpu->status.parsed.isrgb24 = (command.value & 0x00000010) >> 4;
  gpu->status.parsed.isinter = (command.value & 0x00000020) >> 5;
  gpu->status.parsed.width1 = (command.value & 0x00000040) >> 6;
  bool resolutionChanged = false;
  uint32_t screenWidth;
  if (gpu->status.parsed.width1) {
    screenWidth = 368;
  } else {
    switch (gpu->status.parsed.width0) {
    case 0:
      screenWidth = 256;
      break;
    case 1:
      screenWidth = 320;
      break;
    case 2:
      screenWidth = 512;
      break;
    case 3:
      screenWidth = 640;
      break;
    }
  }
  gpu->screenWidth = screenWidth;
  gpu->screenHeight = gpu->status.parsed.height && gpu->status.parsed.isinter ? 480 : 240;
}

void GpuGetGpuInfo(Gpu *gpu, GpuCommand command) { PCFDEBUG("Gpu -> GetGpuInfo"); }

void GpuSetDrawMode(Gpu *gpu, GpuPacket packet) {
  gpu->status.parsed.texx = packet & 0x0000000F;
  gpu->status.parsed.texy = (packet & 0x00000010) >> 4;
  gpu->status.parsed.semitransparencyMode = (packet & 0x00000060) >> 5;
  gpu->status.parsed.texPageColorMode = (packet & 0x00000180) >> 6;
  gpu->status.parsed.dither = (packet & 0x00000200) >> 9;
  gpu->status.parsed.allowDrawToDisplayArea = (packet & 0x00000400) >> 10;
  gpu->status.parsed.texDisable = (packet & 0x00000800) >> 11;
}
void GpuSetTextureWindow(Gpu *gpu, GpuPacket packet) {
  gpu->texWindowMaskX = packet & 0x0000000F;
  gpu->texWindowMaskY = (packet & 0x000000F0) >> 4;
  gpu->texWindowOffsetX = (packet & 0x00000F00) >> 8;
  gpu->texWindowOffsetY = (packet & 0x0000F000) >> 12;
}

void GpuSetDrawingArea(Gpu *gpu, GpuPacket packet) {
  if ((packet & 0xFF000000) == 0xE3000000) {
    gpu->drawingAreaLeft = packet & 0x000003FF;
    gpu->drawingAreaTop = (packet & 0x0007FC00) >> 10;
  } else {
    gpu->drawingAreaRight = packet & 0x000003FF;
    gpu->drawingAreaBottom = (packet & 0x0007FC00) >> 10;
  }
}

void GpuSetDrawingOffset(Gpu *gpu, GpuPacket packet) {
  gpu->drawingAreaOffsetX = packet & 0x000007FF;
  gpu->drawingAreaOffsetY = (packet & 0x003FF800) >> 11;
}

void GpuSetMaskBit(Gpu *gpu, GpuPacket packet) {
  gpu->status.parsed.maskSet = packet & 0x00000001;
  gpu->status.parsed.maskEnable = (packet & 0x00000002) >> 1;
}

static void GpuRenderMonochromeQuadImpl(Gpu *gpu) {
  uint32_t color = GpuPeekPacket(gpu);
  GpuPackedVertex vertex1 = NewPackedVertex(GpuPeekPacketAt(gpu, 1));
  GpuPackedVertex vertex2 = NewPackedVertex(GpuPeekPacketAt(gpu, 2));
  GpuPackedVertex vertex3 = NewPackedVertex(GpuPeekPacketAt(gpu, 3));
  GpuPackedVertex vertex4 = NewPackedVertex(GpuPeekPacketAt(gpu, 4));
  GpuMonochromeTriangle(gpu, vertex1, vertex2, vertex3, color);
  GpuMonochromeTriangle(gpu, vertex3, vertex2, vertex4, color);
}

void GpuRenderMonochromeQuad(Gpu *gpu, GpuPacket packet) {
  gpu->continuation.requiredPackets = 5;
  gpu->continuation.cyclesToRun = 32; //????
  gpu->continuation.runningCommand = GpuRenderMonochromeQuadImpl;
}

static void GpuRenderShadedTriImpl(Gpu *gpu) {
  GpuPackedColor c1 = NewPackedColor(GpuPeekPacket(gpu));
  GpuPackedVertex v1 = NewPackedVertex(GpuPeekPacketAt(gpu, 1));
  GpuPackedColor c2 = NewPackedColor(GpuPeekPacketAt(gpu, 2));
  GpuPackedVertex v2 = NewPackedVertex(GpuPeekPacketAt(gpu, 3));
  GpuPackedColor c3 = NewPackedColor(GpuPeekPacketAt(gpu, 4));
  GpuPackedVertex v3 = NewPackedVertex(GpuPeekPacketAt(gpu, 5));
  GpuShadedTriangle(gpu, v1, v2, v3, c1, c2, c3);
}

void GpuRenderShadedTri(Gpu *gpu, GpuPacket packet) {
  gpu->continuation.requiredPackets = 6;
  gpu->continuation.cyclesToRun = 32; //????
  gpu->continuation.runningCommand = GpuRenderShadedTriImpl;
}

static void GpuRenderShadedQuadImpl(Gpu *gpu) {
  GpuPackedColor c1 = NewPackedColor(GpuPeekPacket(gpu));
  GpuPackedVertex v1 = NewPackedVertex(GpuPeekPacketAt(gpu, 1));
  GpuPackedColor c2 = NewPackedColor(GpuPeekPacketAt(gpu, 2));
  GpuPackedVertex v2 = NewPackedVertex(GpuPeekPacketAt(gpu, 3));
  GpuPackedColor c3 = NewPackedColor(GpuPeekPacketAt(gpu, 4));
  GpuPackedVertex v3 = NewPackedVertex(GpuPeekPacketAt(gpu, 5));
  GpuPackedColor c4 = NewPackedColor(GpuPeekPacketAt(gpu, 6));
  GpuPackedVertex v4 = NewPackedVertex(GpuPeekPacketAt(gpu, 7));
  GpuShadedTriangle(gpu, v1, v2, v3, c1, c2, c3);
  GpuShadedTriangle(gpu, v3, v2, v4, c3, c2, c4);
}

void GpuRenderShadedQuad(Gpu *gpu, GpuPacket packet) {
  gpu->continuation.requiredPackets = 8;
  gpu->continuation.cyclesToRun = 32; //????
  gpu->continuation.runningCommand = GpuRenderShadedQuadImpl;
}

void GpuCopyRectVramVramImpl(Gpu *gpu) {
  GpuPackedVertex source = NewPackedVertex(GpuPeekPacketAt(gpu, 1));
  GpuPackedVertex destination = NewPackedVertex(GpuPeekPacketAt(gpu, 2));
  GpuPackedVertex widthHeight = NewPackedVertex(GpuPeekPacketAt(gpu, 3));
}

void GpuCopyRectVramVram(Gpu *gpu, GpuPacket packet) {
  gpu->continuation.requiredPackets = 4;
  gpu->continuation.cyclesToRun = 32; //????
  gpu->continuation.runningCommand = GpuCopyRectVramVramImpl;
}

void GpuCopyRectCpuVramImpl(Gpu *gpu) {
  GpuPackedVertex destination = NewPackedVertex(GpuPeekPacketAt(gpu, 1));
  GpuPackedVertex widthHeight = NewPackedVertex(GpuPeekPacketAt(gpu, 2));
  gpu->continuation.writeToVram = true;
  gpu->continuation.writeToVramIndex = 0;
  gpu->continuation.writeToVramX = destination.coords.x;
  gpu->continuation.writeToVramY = destination.coords.y;
  gpu->continuation.writeToVramWidth = widthHeight.coords.x;
  gpu->continuation.writeToVramHeight = widthHeight.coords.y;
}

void GpuCopyRectCpuVram(Gpu *gpu, GpuPacket packet) {
  gpu->continuation.requiredPackets = 3;
  gpu->continuation.cyclesToRun = 1; //????
  gpu->continuation.runningCommand = GpuCopyRectCpuVramImpl;
}

void GpuCopyRectVramCpuImpl(Gpu *gpu) {}

void GpuCopyRectVramCpu(Gpu *gpu, GpuPacket packet) {
  gpu->continuation.requiredPackets = 3;
  gpu->continuation.cyclesToRun = 1; //????
  gpu->continuation.runningCommand = GpuCopyRectCpuVramImpl;
}

uint32_t GpuRead32(Gpu *gpu, MemorySegment segment, Address address) {
  if (address == 0) {
    return GpuGetCommandResponse(gpu);
  }
  return GpuGetStatus(gpu);
}

uint16_t GpuRead16(Gpu *gpu, MemorySegment segment, Address address) {
  PCF_PANIC("GpuRead16 not supported! Address: " ADDR_FORMAT, address);
  return 0;
}

uint8_t GpuRead8(Gpu *gpu, MemorySegment segment, Address address) {
  PCF_PANIC("GpuRead8 not supported! Address: " ADDR_FORMAT, address);
  return 0;
}

void GpuWrite32(Gpu *gpu, MemorySegment segment, Address address, uint32_t value) {
  if (address == 0) {
    GpuSendCommand(gpu, value);
  } else {
    GpuSendControl(gpu, value);
  }
}

void GpuWrite16(Gpu *gpu, MemorySegment segment, Address address, uint16_t value) {
  PCF_PANIC("GpuWrite16 not supported! Address: " ADDR_FORMAT ", value = " ADDR_FORMAT, address, (uint32_t)value);
}

void GpuWrite8(Gpu *gpu, MemorySegment segment, Address address, uint8_t value) {
  PCF_PANIC("GpuWrite16 not supported! Address: " ADDR_FORMAT ", value = " ADDR_FORMAT, address, (uint32_t)value);
}

ASSUME_NONNULL_END
