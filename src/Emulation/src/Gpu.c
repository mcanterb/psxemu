#include "Gpu.h"
#include "System.h"

ASSUME_NONNULL_BEGIN

static const size_t kVramSize = 1024 * 512;

typedef union __GpuStatus {
  uint32_t value;
  struct __attribute__((packed)) __GpuStatusParsed {
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
  GpuPacket packetBuffer[16];
  size_t bufferPos;
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
  gpu->status.parsed.ready = 1;
  gpu->status.parsed.comReady = 1;
  gpu->status.parsed.imgReady = 1;
  BusDevice device = GpuBusDevice(gpu);
  PCFResultOrPanic(BusRegisterDevice(
      bus, &device, NewAddressRange(0x1F801810, 0x1F801818, kMainSegments)));
  return gpu;
}

uint32_t GpuGetStatus(Gpu *gpu) { return gpu->status.value; }

uint32_t GpuGetCommandResponse(Gpu *gpu) { return gpu->commandResponse; }
void GpuSendCommand(Gpu *gpu, GpuPacket packet) {}
void GpuSendControl(Gpu *gpu, GpuPacket packet) {}

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

void GpuWrite32(Gpu *gpu, MemorySegment segment, Address address,
                uint32_t value) {
  if (address == 0) {
    GpuSendCommand(gpu, value);
  } else {
    GpuSendControl(gpu, value);
  }
}

void GpuWrite16(Gpu *gpu, MemorySegment segment, Address address,
                uint16_t value) {
  PCF_PANIC("GpuWrite16 not supported! Address: " ADDR_FORMAT
            ", value = " ADDR_FORMAT,
            address, (uint32_t)value);
}

void GpuWrite8(Gpu *gpu, MemorySegment segment, Address address,
               uint8_t value) {
  PCF_PANIC("GpuWrite16 not supported! Address: " ADDR_FORMAT
            ", value = " ADDR_FORMAT,
            address, (uint32_t)value);
}

ASSUME_NONNULL_END
