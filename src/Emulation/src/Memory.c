#include "Memory.h"
#include "System.h"
#include <string.h>

ASSUME_NONNULL_BEGIN

const static size_t kMemorySize = 2 * 1024 * 1024;
const static size_t kMemoryMask = 0x001FFFFF;
const static size_t kDataCacheSize = 1024;

struct __Memory {
  uint8_t memory[];
};

static inline BusDevice MemoryBusDevice(Memory *mem, uint32_t cpuCycles) {
  BusDevice device = {.context = mem,
                      .cpuCycles = cpuCycles,
                      .read32 = (Read32)MemoryRead32,
                      .read16 = (Read16)MemoryRead16,
                      .read8 = (Read8)MemoryRead8,
                      .write32 = (Write32)MemoryWrite32,
                      .write16 = (Write16)MemoryWrite16,
                      .write8 = (Write8)MemoryWrite8};
  return device;
}

static Memory *MemoryNewInternal(System *sys, Bus *bus, size_t size, AddressRange range, uint32_t cycles) {
  Memory *mem = (Memory *)SystemArenaAllocate(sys, size);
  memset(mem->memory, 0xAA, size);
  BusDevice device = MemoryBusDevice(mem, cycles);
  PCFResultOrPanic(BusRegisterDevice(bus, &device, range));
  return mem;
}

Memory *MemoryNew(System *sys, Bus *bus) {
  AddressRange range = NewAddressRange(0x00000000, 0x00800000, kMainSegments);
  return MemoryNewInternal(sys, bus, kMemorySize, range, 3);
}

Memory *DataCacheNew(System *sys, Bus *bus) {
  AddressRange range = NewAddressRange(0x1F800000, 0x1F800400, UserSegment | KernelSegment0);
  return MemoryNewInternal(sys, bus, kDataCacheSize, range, 0);
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
  return ((uint8_t *)mem->memory)[(address & kMemoryMask)];
}

void MemoryWrite32(Memory *mem, MemorySegment segment, Address address, uint32_t data) {
  Address addr = (address & kMemoryMask) >> 2;
  ((uint32_t *)mem->memory)[addr] = data;
}

void MemoryWrite16(Memory *mem, MemorySegment segment, Address address, uint16_t data) {
  Address addr = (address & kMemoryMask) >> 1;
  ((uint16_t *)mem->memory)[addr] = data;
}

void MemoryWrite8(Memory *mem, MemorySegment segment, Address address, uint8_t data) {
  ((uint8_t *)mem->memory)[(address & kMemoryMask)] = data;
}

ASSUME_NONNULL_END
