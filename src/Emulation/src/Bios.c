#include "Bios.h"
#include "Bus.h"
#include "System.h"

ASSUME_NONNULL_BEGIN

static const size_t kBiosSize = 512 * 1024;
static const size_t kBiosMask = 0x0007FFFF;

struct __Bios {
  System *sys;
  uint8_t bios[kBiosSize];
};

static inline BusDevice BiosBusDevice(Bios *bios) {
  BusDevice device = {.context = bios,
                      .cpuCycles = 6,
                      .read32 = (Read32)BiosRead32,
                      .read16 = (Read16)BiosRead16,
                      .read8 = (Read8)BiosRead8,
                      .write32 = (Write32)BiosWrite32,
                      .write16 = (Write16)BiosWrite16,
                      .write8 = (Write8)BiosWrite8};
  return device;
}

Bios *BiosNew(System *sys, Bus *bus, PCFStringRef biosPath) {
  Bios *bios = (Bios *)SystemArenaAllocate(sys, sizeof(Bios));
  PCFDataRef biosData = PCFDataResultOrPanic(PCFDataNewFromFile(biosPath));
  if (PCFDataCopyInto(biosData, bios->bios, kBiosSize) != kBiosSize) {
    PCF_PANIC("Bios size is incorrect! Bios is %d bytes. Expected %d", PCFDataCapacity(biosData), kBiosSize);
  }
  PCFRelease(biosData);
  bios->sys = sys;
  BusDevice device = BiosBusDevice(bios);
  PCFResultOrPanic(BusRegisterDevice(bus, &device, NewAddressRange(0x1FC00000, 0x20000000, kMainSegments)));
  return bios;
}

uint32_t BiosRead32(Bios *bios, MemorySegment segment, Address address) {
  Address addr = (address & kBiosMask) >> 2;
  return ((uint32_t *)bios->bios)[addr];
}

uint16_t BiosRead16(Bios *bios, MemorySegment segment, Address address) {
  Address addr = (address & kBiosMask) >> 1;
  return ((uint16_t *)bios->bios)[addr];
}

uint8_t BiosRead8(Bios *bios, MemorySegment segment, Address address) {
  Address addr = (address & kBiosMask);
  return bios->bios[addr];
}

void BiosWrite32(Bios *bios, MemorySegment segment, Address address, uint32_t data) {}

void BiosWrite16(Bios *bios, MemorySegment segment, Address address, uint16_t data) {}

void BiosWrite8(Bios *bios, MemorySegment segment, Address address, uint8_t data) {}

ASSUME_NONNULL_END
