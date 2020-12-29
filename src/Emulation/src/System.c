#include "System.h"
#include "Bios.h"
#include "Bus.h"
#include "Clock.h"
#include "Cpu/Cpu.h"
#include "Devices.h"
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
  sys->gpu = GpuNew(sys, bus);
  DmaNew(sys, bus);
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

void SystemInterrupt(System *sys, InterruptCode code) {}

void SystemRun(System *sys) { CpuRun(sys->cpu, 571240); }

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

ASSUME_NONNULL_END
