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
uint32_t GpuScreenWidth(Gpu *gpu);
uint32_t GpuScreenHeight(Gpu *gpu);

BUS_DEVICE_FUNCS(Gpu)

// Gpu Commands
typedef void (*_Nullable GpuControlCommandHandler)(Gpu *gpu, GpuCommand command);
typedef void (*_Nullable GpuCommandHandler)(Gpu *gpu, GpuPacket packet);
typedef void (*_Nullable GpuCommandImpl)(Gpu *gpu);

void GpuInvalidControlCommand(Gpu *gpu, GpuCommand command);
void GpuReset(Gpu *gpu, GpuCommand command);
void GpuClearCommandBuffer(Gpu *gpu, GpuCommand command);
void GpuResetIrq(Gpu *gpu, GpuCommand command);
void GpuDisplayEnable(Gpu *gpu, GpuCommand command);
void GpuSetDmaMode(Gpu *gpu, GpuCommand command);
void GpuSetDisplayStart(Gpu *gpu, GpuCommand command);
void GpuSetHorizontalDisplayRange(Gpu *gpu, GpuCommand command);
void GpuSetVerticalDisplayRange(Gpu *gpu, GpuCommand command);
void GpuSetDisplayMode(Gpu *gpu, GpuCommand command);
void GpuGetGpuInfo(Gpu *gpu, GpuCommand command);

void GpuInvalidCommand(Gpu *gpu, GpuPacket packet);
void GpuNoOp(Gpu *gpu, GpuPacket packet);
void GpuRenderMonochromeTri(Gpu *gpu, GpuPacket packet);
void GpuRenderMonochromeQuad(Gpu *gpu, GpuPacket packet);
void GpuRenderTexturedTri(Gpu *gpu, GpuPacket packet);
void GpuRenderTexturedQuad(Gpu *gpu, GpuPacket packet);
void GpuRenderShadedTri(Gpu *gpu, GpuPacket packet);
void GpuRenderShadedQuad(Gpu *gpu, GpuPacket packet);
void GpuRenderShadedTexturedTri(Gpu *gpu, GpuPacket packet);
void GpuRenderShadedTexturedQuad(Gpu *gpu, GpuPacket packet);
void GpuRenderMonochromeLine(Gpu *gpu, GpuPacket packet);
void GpuRenderMonochromePolyLine(Gpu *gpu, GpuPacket packet);
void GpuRenderShadedLine(Gpu *gpu, GpuPacket packet);
void GpuRenderShadedPolyLine(Gpu *gpu, GpuPacket packet);

void GpuRenderMonochromeRectangle(Gpu *gpu, GpuPacket packet);

void GpuRenderMonochromeRectangleVariable(Gpu *gpu, GpuPacket packet);
void GpuRenderTexturedRectangle(Gpu *gpu, GpuPacket packet);
void GpuRenderTexturedRectangleVariable(Gpu *gpu, GpuPacket packet);

void GpuSetDrawMode(Gpu *gpu, GpuPacket packet);
void GpuSetTextureWindow(Gpu *gpu, GpuPacket packet);
void GpuSetDrawingArea(Gpu *gpu, GpuPacket packet);
void GpuSetDrawingOffset(Gpu *gpu, GpuPacket packet);
void GpuSetMaskBit(Gpu *gpu, GpuPacket packet);

void GpuClearCache(Gpu *gpu, GpuPacket packet);
void GpuFillRect(Gpu *gpu, GpuPacket packet);
void GpuCopyRectVramVram(Gpu *gpu, GpuPacket);
void GpuCopyRectCpuVram(Gpu *gpu, GpuPacket);
void GpuCopyRectVramCpu(Gpu *gpu, GpuPacket);

ASSUME_NONNULL_END
