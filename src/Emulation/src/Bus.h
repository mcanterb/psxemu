#pragma once
#include "Types.h"
#include <PsxCoreFoundation/Data.h>
#include <stdbool.h>
#include <stdint.h>

ASSUME_NONNULL_BEGIN

Bus *BusNew(System *sys, size_t maxDevices);
PCFResult BusRegisterDevice(Bus *bus, BusDevice *device,
                            AddressRange addressRange);
uint8_t BusRead8(Bus *bus, Address address,
                 SystemException *_Nonnull *_Nullable exception);
uint16_t BusRead16(Bus *bus, Address address,
                   SystemException *_Nonnull *_Nullable exception);
uint32_t BusRead32(Bus *bus, Address address,
                   SystemException *_Nonnull *_Nullable exception);
void BusWrite8(Bus *bus, Address address, uint8_t value,
               SystemException *_Nonnull *_Nullable exception);
void BusWrite16(Bus *bus, Address address, uint16_t value,
                SystemException *_Nonnull *_Nullable exception);
void BusWrite32(Bus *bus, Address address, uint32_t value,
                SystemException *_Nonnull *_Nullable exception);

ASSUME_NONNULL_END
