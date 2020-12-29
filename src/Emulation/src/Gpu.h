#pragma once
#include "Types.h"

ASSUME_NONNULL_BEGIN

Gpu *GpuNew(System *sys, Bus *bus);
uint32_t GpuGetStatus(Gpu *gpu);
uint32_t GpuGetCommandResponse(Gpu *gpu);
void GpuSendCommand(Gpu *gpu, GpuPacket packet);
void GpuSendControl(Gpu *gpu, GpuPacket packet);
void GpuRun(Gpu *gpu, uint32_t cycles);
void GpuUpdateScreen(Gpu *gpu, GpuScreen screen);
BUS_DEVICE_FUNCS(Gpu)

ASSUME_NONNULL_END
