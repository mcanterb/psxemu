#pragma once
#include "Types.h"
#include <PsxCoreFoundation/Data.h>

ASSUME_NONNULL_BEGIN

Memory *MemoryNewCustom(System *sys, Bus *bus, size_t size, AddressRange range, uint32_t cycles);
Memory *MemoryNew(System *sys, Bus *bus);
BUS_DEVICE_FUNCS(Memory)

ASSUME_NONNULL_END
