#pragma once
#include "Bus.h"
#include "Cpu.h"
#include "Types.h"
#include <PsxCoreFoundation/Data.h>

ASSUME_NONNULL_BEGIN

void SystemInterrupt(System *sys, InterruptCode code);
void SystemRun(System *sys);

System *SystemNew(PCFStringRef biosPath, PCFStringRef cdromPath,
                  PCFStringRef memoryCardPath);

void *SystemArenaAllocate(System *sys, size_t size);

ASSUME_NONNULL_END
