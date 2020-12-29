#pragma once
#include "Types.h"
#include <PsxCoreFoundation/Data.h>
#include <stdbool.h>
#include <stdint.h>

ASSUME_NONNULL_BEGIN

Bus *BusNew(System *sys, size_t maxDevices);
PCFResult BusRegisterDevice(Bus *bus, BusDevice *device, AddressRange addressRange);
bool BusRead8(Bus *bus, Address address, uint8_t *result, SystemException *exception, uint32_t *cycles);
bool BusRead16(Bus *bus, Address address, uint16_t *result, SystemException *exception, uint32_t *cycles);
bool BusRead32(Bus *bus, Address address, uint32_t *result, SystemException *exception, uint32_t *cycles);
bool BusWrite8(Bus *bus, Address address, uint8_t value, SystemException *exception, uint32_t *cycles);
bool BusWrite16(Bus *bus, Address address, uint16_t value, SystemException *exception, uint32_t *cycles);
bool BusWrite32(Bus *bus, Address address, uint32_t value, SystemException *exception, uint32_t *cycles);
void BusDump(Bus *bus, Address start, Address end, PCFStringRef fileName);

ASSUME_NONNULL_END
