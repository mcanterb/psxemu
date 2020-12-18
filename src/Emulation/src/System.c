#include "System.h"
#include "Bus.h"
#include "Cpu.h"
#include "System.h"
#include <stdint.h>

ASSUME_NONNULL_BEGIN

static const size_t kNumOfBusDevices = 10;
static const size_t kSystemArenaSize = 1024 * 1024 * 10;

struct __System {
  size_t arenaPosition;
  Cpu *cpu;
  Bus *bus;
};

System *SystemNew(PCFStringRef biosPath, PCFStringRef cdromPath,
                  PCFStringRef memoryCardPath) {
  void *arena = PCFMalloc(kSystemArenaSize);
  System *sys = (System *)SystemArenaAllocate((System *)arena, sizeof(System));
  sys->bus = BusNew(sys, kNumOfBusDevices);
  sys->cpu = CpuNew(sys, sys->bus);
  return sys;
}

void SystemInterrupt(System *sys, InterruptCode code) {}

void SystemRun(System *sys) {}

void *SystemArenaAllocate(System *sys, size_t size) {
  if (sys->arenaPosition + size >= kSystemArenaSize) {
    PCF_PANIC("Exceeded System Arena Size!");
    return (void *)4;
  }
  sys->arenaPosition += size;
  return ((uint8_t *)sys) + sys->arenaPosition;
}

ASSUME_NONNULL_END
