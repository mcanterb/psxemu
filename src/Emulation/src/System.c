#include "System.h"
#include "Bios.h"
#include "Bus.h"
#include "Clock.h"
#include "Cpu/Cpu.h"
#include "Devices.h"
#include "Dma.h"
#include "Gpu.h"
#include "Memory.h"
#include "System.h"
#include <SDL_surface.h>
#include <stdint.h>

ASSUME_NONNULL_BEGIN

static const size_t kNumOfBusDevices = 29;
static const size_t kSystemArenaSize = 1024 * 1024 * 10;

struct __System {
  size_t arenaPosition;
  Clock *clock;
  Cpu *cpu;
  Bus *bus;
  Gpu *gpu;
  Memory *memory;
  Bios *bios;
  Dma *dma;
};

System *SystemNew(PCFStringRef biosPath, PCFStringRef _Nullable cdromPath, PCFStringRef _Nullable memoryCardPath) {
  void *arena = PCFMalloc(kSystemArenaSize);
  System *sys = (System *)SystemArenaAllocate((System *)arena, sizeof(System));
  sys->clock = ClockNew(sys);
  Bus *bus = BusNew(sys, kNumOfBusDevices);
  sys->bus = bus;
  sys->cpu = CpuNew(sys, bus, sys->clock);
  sys->memory = MemoryNew(sys, bus);
  sys->bios = BiosNew(sys, bus, biosPath);
  CpuRegisterCacheControl(sys->cpu);
  sys->dma = DmaNew(sys, bus);
  sys->gpu = GpuNew(sys, bus);
  TimersNew(sys, bus);
  CdromNew(sys, bus);
  PeripheralsNew(sys, bus);
  InterruptControlNew(sys, bus);
  MemoryControl2New(sys, bus);
  MemoryControl1New(sys, bus);
  Expansion1New(sys, bus);
  Expansion2New(sys, bus);
  SpuControlNew(sys, bus);
  MdecNew(sys, bus);
  SpuVoiceNew(sys, bus);
  SpuMiscNew(sys, bus);
  SpuReverbNew(sys, bus);
  ClockResetRealtime(sys->clock);
  return sys;
}

Clock *SystemClock(System *sys) { return sys->clock; }

Memory *SystemMemory(System *sys) { return sys->memory; }

Dma *SystemDma(System *sys) { return sys->dma; }

void SystemInterrupt(System *sys, InterruptCode code) {}

void SystemRun(System *sys) { CpuRun(sys->cpu, 571240); }

uint32_t SystemDmaRun(System *sys) { return DmaRun(sys->dma); }

bool SystemIsDmaActive(System *sys) { return DmaIsActive(sys->dma); }

void *SystemArenaAllocate(System *sys, size_t size) {
  if (sys->arenaPosition + size >= kSystemArenaSize) {
    PCF_PANIC("Exceeded System Arena Size!");
    return (void *)4; // Should never happen since PCF_PANIC crashes the application.
  }
  void *position = ((uint8_t *)sys) + sys->arenaPosition;
  sys->arenaPosition += size;
  return position;
}

void SystemUpdateSurface(System *sys, SDL_Surface *surface) {
  GpuUpdateScreen(sys->gpu, NewGpuScreen(surface->w, surface->h, surface->pixels));
}

void SystemSync(System *sys) { ClockSyncToRealtime(sys->clock); }

static void SystemDebugMemory(System *sys, const char *str) {
  size_t len = strlen(str);
  if (len == 10 || len == 12) {
    char *endptr;
    uint32_t address = (uint32_t)strtol((str + 2), &endptr, 16);
    uint32_t cycles;
    SystemException exception;
    uint32_t result;
    if (BusRead32(sys->bus, address, &result, &exception, &cycles)) {
      printf("0x%08x: 0x%08x\n", address, result);
    } else {
      printf("Exception: %d\n", exception.code);
    }
  } else {
    printf("Incorrect m command format!\n");
  }
}

void SystemBreakpoint(System *sys) {
  printf("Entering Debugger:\n");
  char buffer[32];
  char command;
  bool quit = false;
  while (!quit) {
    printf("> ");
    if (gets_s(buffer, 32) != NULL) {
      if (strlen(buffer) == 0) {
        printf("Please enter a command.\n");
        continue;
      }
      switch (buffer[0]) {
      case 'x':
        quit = true;
        break;
      case 'r':
        CpuPrintRegs(sys->cpu);
        break;
      case 'm':
        SystemDebugMemory(sys, buffer);
        break;
      case 's':
        CpuPrintStack(sys->cpu);
        break;
      default:
        printf("Unrecognized command format.\n");
      }
    } else {
      printf("Too much input.\n");
    }
  }
}

ASSUME_NONNULL_END
