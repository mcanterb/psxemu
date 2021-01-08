#pragma once
#include "Clock.h"
#include "Types.h"

ASSUME_NONNULL_BEGIN

typedef void (*DmaChannelWrite32)(void *context, Address ramAddress, uint32_t value);
typedef uint32_t (*DmaChannelRead32)(void *context, Address ramAddress);
typedef bool (*DmaChannelIsReady)(void *context);

typedef enum __DmaChannelName {
  DmaChannelMdecIn = 0,
  DmaChannelMdecOut,
  DmaChannelGpu,
  DmaChannelCdrom,
  DmaChannelSpu,
  DmaChannelPio,
  DmaChannelOtc
} DmaChannelName;

Dma *DmaNew(System *sys, Bus *bus);
uint32_t DmaRun(Dma *dma);
DmaChannelPort *DmaGetChannel(Dma *dma, DmaChannelName channelName);
void DmaChannelSetHandlers(DmaChannelPort *channel, void *context, DmaChannelWrite32 write32, DmaChannelRead32 read32,
                           DmaChannelIsReady isReady);
void DmaChannelSetClocksPerWord(DmaChannelPort *channel, uint32_t clocksPerWord);
bool DmaIsActive(Dma *dma);
BUS_DEVICE_FUNCS(Dma)

ASSUME_NONNULL_END
