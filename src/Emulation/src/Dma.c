#include "Dma.h"
#include "Bus.h"
#include "Clock.h"
#include "Memory.h"
#include "System.h"

ASSUME_NONNULL_BEGIN

static const uint32_t kDmaInterruptRegisterWriteMask = 0x7FFF801F;
static const uint32_t kDmaChannelControlRegisterWriteMask = 0x71770703;
static const uint32_t kDmaChannelOtcControlRegisterWriteMask = 0x51000000;
static const uint32_t kDmaBaseAddressRegisterWriteMask = 0x00FFFFFF;
static const uint32_t kDmaBaseAddressRegisterRamMask = 0x001FFFFC;

typedef enum __DmaChannelSyncMode {
  DmaChannelSyncManual = 0,
  DmaChannelSyncRequest,
  DmaChannelSyncLinkedList,
  DmaChannelSyncUnknown
} DmaChannelSyncMode;

typedef struct packed __DmaControlRegParsed {
  uint32_t mdecInPriority : 3;
  uint32_t mdecInEnable : 1;
  uint32_t mdecOutPriority : 3;
  uint32_t mdecOutEnable : 1;
  uint32_t gpuPriority : 3;
  uint32_t gpuEnable : 1;
  uint32_t cdromPriority : 3;
  uint32_t cdromEnable : 1;
  uint32_t spuPriority : 3;
  uint32_t spuEnable : 1;
  uint32_t pioPriority : 3;
  uint32_t pioEnable : 1;
  uint32_t otcPriority : 3;
  uint32_t otcEnable : 1;
  uint32_t unknown28_31 : 4;
} DmaControlRegParsed;

typedef union packed __DmaControlReg {
  uint32_t value;
  DmaControlRegParsed parsed;
} DmaControlReg;

typedef struct packed __DmaInterruptRegParsed {
  uint32_t unknown0_14 : 15;
  uint32_t forceIrq : 1;
  uint32_t mdecInIrqEnable : 1;
  uint32_t mdecOutIrqEnable : 1;
  uint32_t gpuIrqEnable : 1;
  uint32_t cdromIrqEnable : 1;
  uint32_t spuIrqEnable : 1;
  uint32_t pioIrqEnable : 1;
  uint32_t otcIrqEnable : 1;
  uint32_t irqMasterEnable : 1;
  uint32_t mdecInIrqFlag : 1;
  uint32_t mdecOutIrqFlag : 1;
  uint32_t gpuIrqFlag : 1;
  uint32_t cdromIrqFlag : 1;
  uint32_t spuIrqFlag : 1;
  uint32_t pioIrqFlag : 1;
  uint32_t otcIrqFlag : 1;
  uint32_t irqMasterFlag : 1;
} DmaInterruptRegParsed;

typedef union packed __DmaInterruptReg {
  uint32_t value;
  DmaInterruptRegParsed parsed;
} DmaInterruptReg;

typedef struct packed __DmaBlockControlRegParsed {
  uint16_t numWords;
  uint16_t numBlocks;
} DmaBlockControlRegParsed;

typedef union packed __DmaBlockControlReg {
  uint32_t value;
  DmaBlockControlRegParsed parsed;
} DmaBlockControlReg;

typedef struct packed __DmaChannelControlRegParsed {
  uint32_t fromRam : 1;
  uint32_t stepBackward : 1;
  uint32_t zero2_7 : 6;
  uint32_t choppingEnable : 1;
  uint32_t syncMode : 2;
  uint32_t zero11_15 : 5;
  uint32_t chopDmaWindowSize : 3;
  uint32_t zero19 : 1;
  uint32_t chopCpuWindowSize : 3;
  uint32_t zero23 : 1;
  uint32_t startBusy : 1;
  uint32_t zero25_27 : 3;
  uint32_t startTrigger : 1;
  uint32_t unused29_30 : 2;
  uint32_t zero31 : 1;
} DmaChannelControlRegParsed;

typedef union packed __DmaChannelControlReg {
  uint32_t value;
  DmaChannelControlRegParsed parsed;
} DmaChannelControlReg;

typedef struct packed __DmaChannelRegs {
  uint32_t baseAddress;
  DmaBlockControlReg blockControl;
  DmaChannelControlReg channelControl;
  uint32_t unused;
} DmaChannelRegs;

struct __DmaChannelPort {
  void *context;
  uint32_t clocksPerWord;
  DmaChannelName channelName;
  DmaChannelIsReady isReady;
  DmaChannelWrite32 write32;
  DmaChannelRead32 read32;
};

struct __Dma {
  DmaChannelPort channelPorts[7];
  bool isActive;
  DmaChannelName activeChannel;
  Address ramAddress;
  uint32_t blockSize;
  uint32_t blocksLeft;
  bool ignoreReady;
  bool isLinkedListTransfer;
  Memory *memory;
  Clock *clock;
  DmaChannelRegs channelRegs[7];
  DmaControlReg controlReg;
  DmaInterruptReg interruptReg;
};

static inline DmaChannelName GetChannelName(Address address) { return (DmaChannelName)((address & 0x70) >> 4); }
static inline uint32_t GetRegisterOffset(Address address) { return (address & 0xF) >> 2; }
static inline BusDevice DmaBusDevice(Dma *dma) {
  BusDevice device = {.context = dma,
                      .cpuCycles = 0,
                      .read32 = (Read32)DmaRead32,
                      .read16 = (Read16)DmaRead16,
                      .read8 = (Read8)DmaRead8,
                      .write32 = (Write32)DmaWrite32,
                      .write16 = (Write16)DmaWrite16,
                      .write8 = (Write8)DmaWrite8};
  return device;
}

static inline uint32_t GetRamStepFromControlReg(DmaChannelControlReg reg) {
  return reg.parsed.stepBackward ? (uint32_t)-4 : 4;
}

static uint32_t DmaResumeLinkedList(Dma *dma) {
  DmaChannelRegs *regs = &dma->channelRegs[dma->activeChannel];
  DmaChannelPort *port = &dma->channelPorts[dma->activeChannel];
  Address address = dma->ramAddress;
  uint32_t cycles = 0;

  while (port->isReady(port->context) && address != 0x00FFFFFF) {
    uint32_t header = MemoryRead32(dma->memory, UserSegment, address);
    uint32_t size = (header & 0xFF000000) >> 24;
    int i;
    for (i = 0; i < size; i++) {
      address = (address + 4) & kDmaBaseAddressRegisterRamMask;
      uint32_t value = MemoryRead32(dma->memory, UserSegment, address);
      port->write32(port->context, address, value);
    }
    address = header & 0x00FFFFFF;
    ClockTick(dma->clock, port->clocksPerWord * size);
    cycles += port->clocksPerWord * size;
    while (!port->isReady(port->context)) {
      ClockTick(dma->clock, 32);
      cycles += 32;
    }
  }
  dma->ramAddress = address;
  if (address == 0x00FFFFFF) {
    dma->isActive = false;
    regs->channelControl.parsed.startBusy = false;
  }
  return cycles;
}

static void DmaLinkedList(Dma *dma, DmaChannelName channel) {
  dma->isLinkedListTransfer = true;
  dma->ignoreReady = false;
}

static uint32_t DmaResumeBlockTransfer(Dma *dma) {
  DmaChannelPort *port = &dma->channelPorts[dma->activeChannel];
  DmaChannelRegs *regs = &dma->channelRegs[dma->activeChannel];
  bool fromRam = regs->channelControl.parsed.fromRam;
  uint32_t step = GetRamStepFromControlReg(regs->channelControl);

  Address address = dma->ramAddress;
  uint32_t *blocks = &dma->blocksLeft;
  uint32_t blockSize = dma->blockSize;
  bool ignoreReady = dma->ignoreReady;
  uint32_t cycles = 0;

  while (*blocks > 0 && (ignoreReady || port->isReady(port->context))) {
    int i;
    for (i = 0; i < blockSize; i++) {
      if (fromRam) {
        uint32_t value = MemoryRead32(dma->memory, UserSegment, address);
        port->write32(port->context, address, value);
      } else {
        uint32_t value = port->read32(port->context, address);
        MemoryWrite32(dma->memory, UserSegment, address, value);
      }
      address = (address + step) & kDmaBaseAddressRegisterRamMask;
    }
    ClockTick(dma->clock, port->clocksPerWord * blockSize);
    cycles += port->clocksPerWord * blockSize;
    (*blocks)--;
  }
  dma->ramAddress = address;
  if (*blocks == 0) {
    dma->isActive = false;
    regs->channelControl.parsed.startBusy = false;
  }
  return cycles;
}

static void DmaBlockTransfer(Dma *dma, DmaChannelName channel) {
  DmaChannelRegs *regs = &dma->channelRegs[channel];
  dma->isLinkedListTransfer = false;
  switch (regs->channelControl.parsed.syncMode) {
  case DmaChannelSyncManual:
    dma->blocksLeft = 1;
    dma->blockSize = regs->blockControl.parsed.numWords;
    if (regs->blockControl.parsed.numWords == 0) {
      dma->blockSize = 0x10000;
    }
    dma->ignoreReady = true;
    if (channel == DmaChannelOtc) {
      *(uint32_t *)dma->channelPorts[channel].context = dma->blockSize;
    }
    break;
  case DmaChannelSyncRequest:
    dma->blockSize = regs->blockControl.parsed.numWords;
    dma->blocksLeft = regs->blockControl.parsed.numBlocks;
    dma->ignoreReady = false;
    break;
  default:
    PCF_PANIC("Unknown Dma Sync Mode: %d", regs->channelControl.parsed.syncMode);
    return;
  }
}

static void DmaStartDma(Dma *dma, DmaChannelName channel) {
  DmaChannelRegs *regs = &dma->channelRegs[channel];
  dma->isActive = true;
  dma->activeChannel = channel;
  dma->ramAddress = regs->baseAddress & kDmaBaseAddressRegisterRamMask;
  regs->channelControl.parsed.startTrigger = 0;
  if (regs->channelControl.parsed.syncMode == 2) {
    DmaLinkedList(dma, channel);
  } else {
    DmaBlockTransfer(dma, channel);
  }
}

uint32_t OtcDmaChannelRead32(void *context, Address address) {
  uint32_t *remaining = (uint32_t *)context;
  (*remaining)--;
  if (*remaining == 0) {
    return 0xFFFFFF;
  }
  return (address - 4) & kDmaBaseAddressRegisterRamMask;
}

void OtcDmaChannelWrite32(void *context, Address address, uint32_t value) {
  PCF_PANIC("Dma Channel 6 (OTC) does not support writes!");
}

bool OtcDmaChannelIsReady(void *context) { return true; }

Dma *DmaNew(System *sys, Bus *bus) {
  Dma *dma = (Dma *)SystemArenaAllocate(sys, sizeof(*dma));
  uint32_t *otcCounter = (uint32_t *)SystemArenaAllocate(sys, sizeof(uint32_t));
  dma->controlReg.value = 0x07654321;
  dma->channelRegs[DmaChannelOtc].channelControl.parsed.stepBackward = true;
  dma->clock = SystemClock(sys);
  dma->memory = SystemMemory(sys);
  BusDevice device = DmaBusDevice(dma);
  BusRegisterDevice(bus, &device, NewAddressRange(0x1F801080, 0x1F801100, kMainSegments));
  DmaChannelPort *port = DmaGetChannel(dma, DmaChannelOtc);
  DmaChannelSetHandlers(port, otcCounter, OtcDmaChannelWrite32, OtcDmaChannelRead32, OtcDmaChannelIsReady);
  DmaChannelSetClocksPerWord(port, 1);
  return dma;
}

uint32_t DmaRun(Dma *dma) {
  if (dma->isActive) {
    if (dma->isLinkedListTransfer) {
      return DmaResumeLinkedList(dma);
    } else {
      return DmaResumeBlockTransfer(dma);
    }
  }
  return 0;
}

DmaChannelPort *DmaGetChannel(Dma *dma, DmaChannelName channelName) { return &dma->channelPorts[channelName]; }

void DmaChannelSetHandlers(DmaChannelPort *channel, void *context, DmaChannelWrite32 write32, DmaChannelRead32 read32,
                           DmaChannelIsReady isReady) {
  channel->context = context;
  channel->write32 = write32;
  channel->read32 = read32;
  channel->isReady = isReady;
}
void DmaChannelSetClocksPerWord(DmaChannelPort *channel, uint32_t clocksPerWord) {
  channel->clocksPerWord = clocksPerWord;
}

bool DmaIsActive(Dma *dma) { return dma->isActive; }

uint32_t DmaRead32(Dma *dma, MemorySegment segment, Address address) {
  uint32_t reg = GetRegisterOffset(address);
  if (address < 0x70) {
    DmaChannelName channelName = GetChannelName(address);
    switch (reg) {
    case 0:
      return dma->channelRegs[channelName].baseAddress;
    case 1:
      return dma->channelRegs[channelName].blockControl.value;
    case 2:
      return dma->channelRegs[channelName].channelControl.value;
    default:
      return dma->channelRegs[channelName].unused;
    }
  }
  switch (reg) {
  case 0:
    return dma->controlReg.value;
  case 1:
    return dma->interruptReg.value;
  case 2:
    return 0x7FFAC68B; // Probably garbage
  case 3:
    return 0x00FFFFF7; // Probably garbage
  }
  return 0;
}

uint16_t DmaRead16(Dma *dma, MemorySegment segment, Address address) {
  PCF_PANIC("DmaRead16 is unsupported!");
  return 0;
}

uint8_t DmaRead8(Dma *dma, MemorySegment segment, Address address) {
  PCF_PANIC("DmaRead8 is unsupported!");
  return 0;
}

void DmaWrite32(Dma *dma, MemorySegment segment, Address address, uint32_t data) {
  uint32_t reg = GetRegisterOffset(address);
  if (address < 0x70) {
    DmaChannelName channelName = GetChannelName(address);
    switch (reg) {
    case 0:
      dma->channelRegs[channelName].baseAddress = data & kDmaBaseAddressRegisterWriteMask;
      return;
    case 1:
      dma->channelRegs[channelName].blockControl.value = data;
      return;
    case 2: {
      uint32_t mask = kDmaChannelControlRegisterWriteMask;
      DmaChannelControlReg *controlReg = &dma->channelRegs[channelName].channelControl;
      if (channelName == DmaChannelOtc) {
        mask = kDmaChannelOtcControlRegisterWriteMask;
      }
      controlReg->value = data & mask;
      dma->channelRegs[DmaChannelOtc].channelControl.parsed.stepBackward = true;
      if (controlReg->parsed.startBusy && (controlReg->parsed.syncMode != 0 || controlReg->parsed.startTrigger)) {
        DmaStartDma(dma, channelName);
      }
      return;
    }
    default:
      dma->channelRegs[channelName].unused = data;
      return;
    }
  }
  switch (reg) {
  case 0:
    dma->controlReg.value = data;
    return;
  case 1:
    dma->interruptReg.value = data;
    // TODO: Handle DMA interrupts correctly
    return;
  }
}

void DmaWrite16(Dma *dma, MemorySegment segment, Address address, uint16_t data) {
  PCF_PANIC("DmaWrite16 is unsupported!");
}

void DmaWrite8(Dma *dma, MemorySegment segment, Address address, uint8_t data) {
  PCF_PANIC("DmaWrite8 is unsupported!");
}

ASSUME_NONNULL_END
