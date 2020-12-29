#include "Bus.h"
#include "System.h"
#include <PsxCoreFoundation/String.h>
#include <string.h>
ASSUME_NONNULL_BEGIN

typedef struct __BusDeviceEntry {
  BusDevice device;
  AddressRange addressRange;
  struct __BusDeviceEntry *_Nullable left;
  struct __BusDeviceEntry *_Nullable right;
} BusDeviceEntry;

struct __Bus {
  System *sys;
  uint8_t maxDevices;
  uint8_t numDevices;
  BusDeviceEntry devices[];
};

static BusDevice *_Nullable _FindDeviceHelper(BusDeviceEntry *_Nullable entry, Address addr, Address *deviceAddress) {
  if (entry == NULL) {
    return NULL;
  }
  if (InRange(&entry->addressRange, addr)) {
    *deviceAddress = PHYSICAL(addr) - entry->addressRange.start;
    return &entry->device;
  }
  Address addrPhysical = PHYSICAL(addr);
  if (addrPhysical < entry->addressRange.start) {
    return _FindDeviceHelper(entry->left, addr, deviceAddress);
  }
  return _FindDeviceHelper(entry->right, addr, deviceAddress);
}

inline BusDevice *_Nullable _FindDevice(Bus *bus, Address addr, Address *deviceAddress) {
  assert(bus->numDevices != 0);
  return _FindDeviceHelper(bus->devices, addr, deviceAddress);
}

static PCFResult _BusRegisterDeviceHelper(BusDeviceEntry *parent, BusDeviceEntry *target) {
  int32_t result = RangeCompare(target->addressRange, parent->addressRange);
  if (result == 0) {
    return PCFResultError(PCFFORMAT("Duplicate Address Range attached to Bus: " ADDR_FORMAT " - " ADDR_FORMAT,
                                    target->addressRange.start, target->addressRange.end));
  }
  if (result < 0) {
    if (parent->left == NULL) {
      parent->left = target;
      return PCFResultSuccess();
    }
    return _BusRegisterDeviceHelper(parent->left, target);
  }
  if (parent->right == NULL) {
    parent->right = target;
    return PCFResultSuccess();
  }
  return _BusRegisterDeviceHelper(parent->right, target);
}

Bus *BusNew(System *sys, size_t maxDevices) {
  Bus *bus = (Bus *)SystemArenaAllocate(sys, maxDevices * sizeof(BusDeviceEntry) + sizeof(Bus));
  bus->maxDevices = maxDevices;
  bus->sys = sys;
  return bus;
}

PCFResult BusRegisterDevice(Bus *bus, BusDevice *device, AddressRange addressRange) {
  if (bus->numDevices + 1 == bus->maxDevices) {
    return PCFResultError(PCFCSTR("Too many bus devices registered!"));
  }
  size_t entryNum = bus->numDevices;
  bus->devices[entryNum].device = *device;
  bus->devices[entryNum].addressRange = addressRange;
  bus->devices[entryNum].left = NULL;
  bus->devices[entryNum].right = NULL;
  PCFResult result = PCFResultSuccess();
  if (entryNum != 0) {
    result = _BusRegisterDeviceHelper(bus->devices, &bus->devices[entryNum]);
  }
  if (result.successful) {
    bus->numDevices++;
  }
  return result;
}

inline bool IsAddressMisaligned(uint32_t size, Address address) {
  switch (size) {
  case 32:
    return (address & 0x3);
  case 16:
    return (address & 0x1);
  case 8:
    return false;
  default:
    PCF_PANIC("Unsupported BusDevice size: %d", size);
    return false;
  }
}

bool BusRead32(Bus *bus, Address address, uint32_t *result, SystemException *exception, uint32_t *cycles) {
  Address offset;
  BusDevice *_Nullable device = _FindDevice(bus, address, &offset);
  if (device == NULL) {
    *exception = NewSystemException(kExceptionBusErrorFetch, address);
    return false;
  }
  if (IsAddressMisaligned(32, address)) {
    *exception = NewSystemException(kExceptionAddressErrorFetch, address);
    return false;
  }
  *result = device->read32(device->context, MemorySegmentForAddress(address), offset);
  *cycles = device->cpuCycles;
  return true;
}

bool BusRead16(Bus *bus, Address address, uint16_t *result, SystemException *exception, uint32_t *cycles) {
  Address offset;
  BusDevice *_Nullable device = _FindDevice(bus, address, &offset);
  if (device == NULL) {
    *exception = NewSystemException(kExceptionBusErrorFetch, address);
    return false;
  }
  if (IsAddressMisaligned(16, address)) {
    *exception = NewSystemException(kExceptionAddressErrorFetch, address);
    return false;
  }
  *result = device->read16(device->context, MemorySegmentForAddress(address), offset);
  *cycles = device->cpuCycles;
  return true;
}

bool BusRead8(Bus *bus, Address address, uint8_t *result, SystemException *exception, uint32_t *cycles) {
  Address offset;
  BusDevice *_Nullable device = _FindDevice(bus, address, &offset);
  if (device == NULL) {
    *exception = NewSystemException(kExceptionBusErrorFetch, address);
    return false;
  }
  if (IsAddressMisaligned(8, address)) {
    *exception = NewSystemException(kExceptionAddressErrorFetch, address);
    return false;
  }
  *result = device->read8(device->context, MemorySegmentForAddress(address), offset);
  *cycles = device->cpuCycles;
  return true;
}

bool BusWrite32(Bus *bus, Address address, uint32_t value, SystemException *exception, uint32_t *cycles) {
  Address offset;
  BusDevice *_Nullable device = _FindDevice(bus, address, &offset);
  if (device == NULL) {
    *exception = NewSystemException(kExceptionBusErrorFetch, address);
    return false;
  }
  if (IsAddressMisaligned(32, address)) {
    *exception = NewSystemException(kExceptionAddressErrorFetch, address);
    return false;
  }
  device->write32(device->context, MemorySegmentForAddress(address), offset, value);
  *cycles = device->cpuCycles;
  return true;
}

bool BusWrite16(Bus *bus, Address address, uint16_t value, SystemException *exception, uint32_t *cycles) {
  Address offset;
  BusDevice *_Nullable device = _FindDevice(bus, address, &offset);
  if (device == NULL) {
    *exception = NewSystemException(kExceptionBusErrorFetch, address);
    return false;
  }
  if (IsAddressMisaligned(16, address)) {
    *exception = NewSystemException(kExceptionAddressErrorFetch, address);
    return false;
  }
  device->write16(device->context, MemorySegmentForAddress(address), offset, value);
  *cycles = device->cpuCycles;
  return true;
}

bool BusWrite8(Bus *bus, Address address, uint8_t value, SystemException *exception, uint32_t *cycles) {
  Address offset;
  BusDevice *_Nullable device = _FindDevice(bus, address, &offset);
  if (device == NULL) {
    *exception = NewSystemException(kExceptionBusErrorFetch, address);
    return false;
  }
  if (IsAddressMisaligned(8, address)) {
    *exception = NewSystemException(kExceptionAddressErrorFetch, address);
    return false;
  }
  if (address == 0x1F802041) {
    PCFDEBUG("PSX BIOS: TraceStep - %d", value);
  }
  device->write8(device->context, MemorySegmentForAddress(address), offset, value);
  *cycles = device->cpuCycles;
  return true;
}

void BusDump(Bus *bus, Address start, Address end, PCFStringRef fileName) {
  FILE *file;
  errno_t error = fopen_s(&file, PCFStringToCString(fileName), "w");
  if (error != 0) {
    PCF_PANIC("%s", PCFStringToCString(PCFStringNewFromError(error)));
  }
  Address current;

  for (current = start; current < end; current += 4) {
    SystemException exception;
    uint32_t cycles = 0;
    uint32_t instruction = 0;
    if (!BusRead32(bus, current, &instruction, &exception, &cycles)) {
      PCF_PANIC("Exception dumping bus. Are addresses aligned?");
    }
    fprintf(file, "%08x: %08x\n", current, instruction);
  }
  fclose(file);
}

ASSUME_NONNULL_END
