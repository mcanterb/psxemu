#pragma once
#include "Types.h"
#include <PsxCoreFoundation/Data.h>

ASSUME_NONNULL_BEGIN

Memory *MemoryNew(System *sys, Bus *bus);
BUS_DEVICE_FUNCS(Memory)

ASSUME_NONNULL_END
