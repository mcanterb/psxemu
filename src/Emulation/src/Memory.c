#include "Memory.h"
#include "System.h"
#include <string.h>

ASSUME_NONNULL_BEGIN

const static size_t kMemorySize = 2 * 1024 * 1024;
const static size_t kMemoryMask = 0x001FFFFF;

struct __Memory {
  System *sys;
  uint8_t memory[kMemorySize];
};

static inline BusDevice MemoryBusDevice(Memory *mem) {
  BusDevice device = {.context = mem,
                      .read32 = (Read32)MemoryRead32,
                      .read16 = (Read16)MemoryRead16,
                      .read8 = (Read8)MemoryRead8,
                      .write32 = (Write32)MemoryWrite32,
                      .write16 = (Write16)MemoryWrite16,
                      .write8 = (Write8)MemoryWrite8};
  return device;
}

Memory *MemoryNew(System *sys, Bus *bus) {
  Memory *mem = (Memory *)SystemArenaAllocate(sys, sizeof(Memory));
  mem->sys = sys;
  memset(mem->memory, 0xAA, kMemorySize);
  BusDevice device = MemoryBusDevice(mem);
  PCFResultOrPanic(BusRegisterDevice(
      bus, &device, NewAddressRange(0x00000000, 0x00800000, kMainSegments)));
  return mem;
}

uint32_t MemoryRead32(Memory *mem, MemorySegment segment, Address address) {
  Address addr = (address & kMemoryMask) >> 2;
  return ((uint32_t *)mem->memory)[addr];
}

uint16_t MemoryRead16(Memory *mem, MemorySegment segment, Address address) {
  Address addr = (address & kMemoryMask) >> 1;
  return ((uint16_t *)mem->memory)[addr];
}

uint8_t MemoryRead8(Memory *mem, MemorySegment segment, Address address) {
  return ((uint16_t *)mem->memory)[(address & kMemoryMask)];
}

void MemoryWrite32(Memory *mem, MemorySegment segment, Address address,
                   uint32_t data) {
  Address addr = (address & kMemoryMask) >> 2;
  ((uint32_t *)mem->memory)[addr] = data;
}

void MemoryWrite16(Memory *mem, MemorySegment segment, Address address,
                   uint16_t data) {
  Address addr = (address & kMemoryMask) >> 1;
  ((uint16_t *)mem->memory)[addr] = data;
}

void MemoryWrite8(Memory *mem, MemorySegment segment, Address address,
                  uint8_t data) {
  ((uint8_t *)mem->memory)[(address & kMemoryMask)] = data;
}

ASSUME_NONNULL_END
