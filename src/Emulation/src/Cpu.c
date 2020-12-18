#include "Cpu.h"
#include "System.h"

static const Address kResetVector = 0xBFC00000;
static const Address kGeneralExceptionVector = 0x80000080;

typedef struct __CpuCop0 {
  uint32_t sr;
  uint32_t badVaddr;
  uint32_t cause;
  uint32_t epc;
} CpuCop0;

struct __Cpu {
  Bus *bus;
  uint32_t reg[32];
  uint32_t loadReg[32];
  uint32_t pc;
  union {
    struct __attribute__((packed)) HiLoDistinct {
      uint32_t lo;
      uint32_t hi;
    } distinct;
    uint64_t combined;
  } hilo;
  CpuCop0 cop0;
};

Cpu *CpuNew(System *sys, Bus *bus) {
  Cpu *cpu = (Cpu *)SystemArenaAllocate(sys, sizeof(Cpu));
  cpu->bus = bus;
  int i;
  for (i = 0; i < 32; i++) {
    cpu->reg[i] = 0xDEADBEEF;
    cpu->loadReg[i] = 0xDEADBEEF;
  }
  return cpu;
}
