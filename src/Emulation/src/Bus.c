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
  SystemException exception;
  uint8_t maxDevices;
  uint8_t numDevices;
  BusDeviceEntry devices[];
};

static BusDevice *_Nullable _FindDeviceHelper(BusDeviceEntry *_Nullable entry,
                                              Address addr,
                                              Address *deviceAddress) {
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

inline BusDevice *_Nullable _FindDevice(Bus *bus, Address addr,
                                        Address *deviceAddress) {
  assert(bus->numDevices != 0);
  return _FindDeviceHelper(bus->devices, addr, deviceAddress);
}

static PCFResult _BusRegisterDeviceHelper(BusDeviceEntry *parent,
                                          BusDeviceEntry *target) {
  int32_t result = RangeCompare(target->addressRange, parent->addressRange);
  if (result == 0) {
    return PCFResultError(
        PCFFORMAT("Duplicate Address Range attached to Bus: " ADDR_FORMAT
                  " - " ADDR_FORMAT,
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
  Bus *bus = (Bus *)SystemArenaAllocate(
      sys, maxDevices * sizeof(BusDeviceEntry) + sizeof(Bus));
  bus->maxDevices = maxDevices;
  bus->sys = sys;
  return bus;
}

PCFResult BusRegisterDevice(Bus *bus, BusDevice *device,
                            AddressRange addressRange) {
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
    return (address & 0xFFFFFF00);
  case 16:
    return (address & 0xFFFFFFF0);
  case 8:
    return false;
  default:
    PCF_PANIC("Unsupported BusDevice size: %d", size);
    return false;
  }
}

#define BUS_READ(size)                                                         \
  uint##size##_t BusRead##size(                                                \
      Bus *bus, Address address,                                               \
      SystemException *_Nonnull *_Nullable exception) {                        \
    Address offset;                                                            \
    BusDevice *_Nullable device = _FindDevice(bus, address, &offset);          \
    if (device == NULL) {                                                      \
      bus->exception = NewSystemException(kExceptionBusErrorFetch, address);   \
      *exception = &bus->exception;                                            \
      return 0;                                                                \
    }                                                                          \
    if (IsAddressMisaligned(size, address)) {                                  \
      bus->exception =                                                         \
          NewSystemException(kExceptionAddressErrorFetch, address);            \
      *exception = &bus->exception;                                            \
      return 0;                                                                \
    }                                                                          \
    *exception = NULL;                                                         \
    return device->read##size(device->context,                                 \
                              MemorySegmentForAddress(address), offset);       \
  }

#define BUS_WRITE(size)                                                        \
  void BusWrite##size(Bus *bus, Address address, uint##size##_t value,         \
                      SystemException *_Nonnull *_Nullable exception) {        \
    Address offset;                                                            \
    BusDevice *_Nullable device = _FindDevice(bus, address, &offset);          \
    if (device == NULL) {                                                      \
      bus->exception = NewSystemException(kExceptionBusErrorStore, address);   \
      *exception = &bus->exception;                                            \
      return;                                                                  \
    }                                                                          \
    if (IsAddressMisaligned(size, address)) {                                  \
      bus->exception =                                                         \
          NewSystemException(kExceptionAddressErrorStore, address);            \
      *exception = &bus->exception;                                            \
      return;                                                                  \
    }                                                                          \
    *exception = NULL;                                                         \
    device->write##size(device->context, MemorySegmentForAddress(address),     \
                        offset, value);                                        \
  }

BUS_READ(32)
BUS_WRITE(32)
BUS_READ(16)
BUS_WRITE(16)
BUS_READ(8)
BUS_WRITE(8)

ASSUME_NONNULL_END
