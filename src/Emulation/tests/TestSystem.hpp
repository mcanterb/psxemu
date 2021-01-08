#pragma once
extern "C" {

#include "../src/Bus.h"
#include "../src/Clock.h"
#include "../src/Cpu/Cpu.h"
#include "../src/Memory.h"
#include "../src/System.h"
#include "../src/Types.h"
}

#include <memory>

// A small mock system struct to use for testing. It cannot be used for every
// system function, but it should suffice for testing the CPU.
typedef struct __TestSystem {
  size_t arenaPosition;
  Clock *clock;
  Cpu *cpu;
  Bus *bus;
  Gpu *gpu;
  Memory *memory;
  Bios *bios;
} TestSystem;

typedef struct __TestProgram {
  uint32_t cyclesToRun;
  size_t size;
  uint8_t *program;
} TestProgram;

typedef std::unique_ptr<TestSystem, decltype(std::free) *> TestSystemUniquePtr;

static void LoadTestProgram(TestSystemUniquePtr &sys, TestProgram program) {
  void *mem = MemoryNewCustom((System *)sys.get(), sys->bus, program.size + 4,
                              NewAddressRange(0x1FC00000, 0x1FC00000 + program.size + 4, kMainSegments), 0);
  memcpy(mem, program.program, program.size);
}

static TestSystemUniquePtr TestSystemNew() {
  void *arena = PCFMalloc(10 * 1024 * 1024);
  TestSystem *testSys = (TestSystem *)arena;
  System *sys = (System *)testSys;
  testSys->arenaPosition = sizeof(*testSys);
  testSys->clock = ClockNew((System *)sys);
  testSys->bus = BusNew((System *)sys, 3);
  testSys->memory = MemoryNew(sys, testSys->bus);
  testSys->cpu = CpuNew(sys, testSys->bus, testSys->clock);
  TestSystemUniquePtr result{testSys, std::free};
  return result;
}
