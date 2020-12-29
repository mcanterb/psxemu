#include "Gpu.h"
#include "Clock.h"
#include "System.h"
#include <immintrin.h>
#include <string.h>

ASSUME_NONNULL_BEGIN

#define kCommandBufferSize 16

static const size_t kVramSize = 1024 * 512;
static const uint32_t kGpuClockRate = 53690000;

static void FastFill(uint32_t *dest, uint32_t value, size_t size) {
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

typedef union __GpuStatus {
  uint32_t value;
  struct packed __GpuStatusParsed {
    uint32_t texx : 4; // x = texx * 64
    uint32_t texy : 1; // 0=0; 1=256
    uint32_t abr : 2;  // Semi transparent state
    uint32_t tp : 2;   // 00=4bit CLUT; 01=8-Bit CLUT; 10=15bit
    uint32_t dither : 1;
    uint32_t dfe : 1; // Draw to display area allowed = 1
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
    uint32_t den : 1;
    uint32_t interrupt : 1;
    uint32_t dmaReq : 1;
    uint32_t ready : 1;
    uint32_t imgReady : 1;
    uint32_t comReady : 1;
    uint32_t dma : 2;
    uint32_t lcf : 1; // 0 when drawing even lines in interlace mode
  } parsed;
} GpuStatus;

struct __Gpu {
  System *sys;
  ClockDeviceHandle clockHandle;
  GpuPacket packetBuffer[kCommandBufferSize];
  size_t bufferEnd;
  size_t bufferStart;
  GpuStatus status;
  uint32_t commandResponse;
  uint16_t displayStartX;
  uint16_t displayStartY;
  uint16_t drawingAreaLeft;
  uint16_t drawingAreaRight;
  uint16_t drawingAreaTop;
  uint16_t drawingAreaBottom;
  uint16_t vram[kVramSize];
};

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

Gpu *GpuNew(System *sys, Bus *bus) {
  Gpu *gpu = (Gpu *)SystemArenaAllocate(sys, sizeof(*gpu));
  gpu->sys = sys;
  gpu->status.parsed.ready = 1;
  gpu->status.parsed.comReady = 1;
  gpu->status.parsed.imgReady = 1;
  BusDevice device = GpuBusDevice(gpu);
  PCFResultOrPanic(BusRegisterDevice(bus, &device, NewAddressRange(0x1F801810, 0x1F801818, kMainSegments)));
  ClockDevice clockDevice = NewClockDevice(gpu, (UpdateHandler)GpuRun, kGpuClockRate);
  gpu->clockHandle = ClockAddDevice(SystemClock(sys), &clockDevice);
  return gpu;
}

uint32_t GpuGetStatus(Gpu *gpu) { return gpu->status.value; }

uint32_t GpuGetCommandResponse(Gpu *gpu) { return gpu->commandResponse; }
void GpuSendCommand(Gpu *gpu, GpuPacket packet) {}
void GpuSendControl(Gpu *gpu, GpuPacket packet) {}

void GpuRun(Gpu *gpu, uint32_t maxCycles) { /*PCFDEBUG("Ran GPU for %d cycles", maxCycles);*/
}

void GpuUpdateScreen(Gpu *gpu, GpuScreen screen) {
  FastFill(screen.pixels, 0x0018499E, (size_t)screen.width * (size_t)screen.height);
  screen.pixels[0] = 0xFFFF0000;
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
