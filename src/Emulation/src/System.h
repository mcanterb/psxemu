#pragma once
#include "Bus.h"
#include "Cpu/Cpu.h"
#include "Types.h"
#include <PsxCoreFoundation/Data.h>
#include <SDL_surface.h>

ASSUME_NONNULL_BEGIN

System *SystemNew(PCFStringRef biosPath, PCFStringRef _Nullable cdromPath, PCFStringRef _Nullable memoryCardPath);
void SystemInterrupt(System *sys, InterruptCode code);
void SystemRun(System *sys);
void SystemUpdateSurface(System *sys, SDL_Surface *surface);
void SystemSync(System *sys);
Clock *SystemClock(System *sys);

void *SystemArenaAllocate(System *sys, size_t size);

ASSUME_NONNULL_END
