#pragma once
#include "Types.h"
#include <PsxCoreFoundation/Data.h>

ASSUME_NONNULL_BEGIN

Bios *BiosNew(System *sys, Bus *bus, PCFStringRef biosPath);
BUS_DEVICE_FUNCS(Bios)

ASSUME_NONNULL_END
