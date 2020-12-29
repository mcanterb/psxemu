#pragma once
#include "Instructions.h"
#include "Types.h"

ASSUME_NONNULL_BEGIN

Cpu *CpuNew(System *sys, Bus *bus, Clock *clock);
void CpuRegisterCacheControl(Cpu *cpu);
void CpuRun(Cpu *cpu, uint32_t cycles);

ASSUME_NONNULL_END
