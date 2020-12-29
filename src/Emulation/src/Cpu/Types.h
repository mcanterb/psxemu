#pragma once
#include "..\Types.h"

ASSUME_NONNULL_BEGIN

#define SIGN_EXTEND_8(val) ((int32_t)(int8_t)val)
#define SIGN_EXTEND_16(val) ((int32_t)(int16_t)val)
#define SIGN_EXTEND(val)                                                                                               \
  _Generic(val, uint8_t                                                                                                \
           : SIGN_EXTEND_8(val), int8_t                                                                                \
           : SIGN_EXTEND_8(val), int16_t                                                                               \
           : SIGN_EXTEND_16(val), uint16_t                                                                             \
           : SIGN_EXTEND_16(val))

static const Address kResetVector = 0xBFC00000;
static const Address kGeneralExceptionVector = 0x00000080;
static const uint32_t kProcessorId = 0x00000002;

typedef struct packed __ImmediateInstruction {
  uint16_t immediate : 16;
  uint16_t rt : 5;
  uint16_t rs : 5;
  uint16_t op : 6;
} ImmediateInstruction;

typedef struct packed __JumpInstruction {
  uint32_t target : 26;
  uint32_t op : 6;
} JumpInstruction;

typedef struct packed __RegisterInstruction {
  uint32_t funct : 6;
  uint32_t shamt : 5;
  uint32_t rd : 5;
  uint32_t rt : 5;
  uint32_t rs : 5;
  uint32_t op : 6;
} RegisterInstruction;

typedef struct packed __CoprocessorRegInstruction {
  uint32_t unused0_11 : 11;
  uint32_t rdCop : 5;
  uint32_t rt : 5;
  uint32_t subOpcode : 4;
  uint32_t unused25 : 1;
  Coprocessor coprocessor : 2;
  uint32_t op : 4;
} CoprocessorRegInstruction;

typedef struct packed __CoprocessorBranchInstruction {
  uint32_t target : 16;
  uint32_t branchOnTrue : 5;
  uint32_t subOpcode : 4;
  uint32_t unused25 : 1;
  Coprocessor coprocessor : 2;
  uint32_t op : 4;
} CoprocessorBranchInstruction;

typedef struct packed __CoprocessorCommandInstruction {
  uint32_t command : 25;
  uint32_t unused25 : 1;
  Coprocessor coprocessor : 2;
  uint32_t op : 4;
} CoprocessorCommandInstruction;

typedef struct packed __CoprocessorLoadStoreInstruction {
  uint32_t immediate : 16;
  uint32_t rtDat : 5;
  uint32_t rs : 5;
  Coprocessor coprocessor : 2;
  uint32_t op : 4;
} CoprocessorLoadStoreInstruction;

typedef union packed __Instruction {
  ImmediateInstruction imm;
  JumpInstruction jump;
  RegisterInstruction reg;
  CoprocessorRegInstruction copReg;
  CoprocessorBranchInstruction copBranch;
  CoprocessorCommandInstruction copCommand;
  CoprocessorLoadStoreInstruction copLoadStore;
  uint32_t value;
} Instruction;

typedef struct __CacheLine {
  union {
    uint32_t value;
    struct packed __CacheLineTagValid {
      uint32_t unused0_1 : 2;
      uint32_t validIndex : 2;
      uint32_t isValid : 1;
      uint32_t unused5_12 : 8;
      uint32_t tag : 19;

    } parsed;
  } tagValid;
  Instruction entries[4];
} CacheLine;

typedef struct packed __CpuCop0 {
  union packed {
    uint32_t value;
    struct packed __CpuCop0SR {
      uint32_t currentInteruptEnable : 1;
      uint32_t currentUserMode : 1;
      uint32_t prevInteruptEnable : 1;
      uint32_t prevUserMode : 1;
      uint32_t oldInteruptEnable : 1;
      uint32_t oldUserMode : 1;
      uint32_t unused6_7 : 2;
      uint32_t interruptMasks : 8;
      uint32_t cacheIsolated : 1;
      uint32_t swapCaches : 1;
      uint32_t writeParity0 : 1;
      uint32_t dataCacheHit : 1;
      uint32_t cacheParityError : 1;
      uint32_t tlbShutdown : 1;
      uint32_t bootExceptionVectors : 1;

      uint32_t unused23_27 : 5;
      uint32_t cop0Enable : 1;
      uint32_t cop1Enable : 1;
      uint32_t cop2Enable : 1;
      uint32_t cop3Enable : 1;
    } parsed;
  } sr;
  union packed {
    uint32_t value;
    struct packed __CpuCop0Cause {
      uint32_t unused0_1 : 2;
      uint32_t exceptionCode : 5;
      uint32_t unused7 : 1;
      uint32_t interruptPending : 8;
      uint32_t unused16_27 : 12;
      uint32_t coprocessor : 2;
      uint32_t unused30 : 1;
      uint32_t branch : 1;
    } parsed;
  } cause;
  uint32_t badVaddr;
  uint32_t epc;
} CpuCop0;

struct __Cpu {
  Bus *bus;
  uint32_t reg[32];
  uint8_t loadReg : 5;
  uint32_t loadValue;
  Address pc;
  Address nextPc;
  Address currentPc;
  bool branch;
  bool delaySlot;
  union {
    struct packed HiLoDistinct {
      uint32_t lo;
      uint32_t hi;
    } distinct;
    uint64_t combined;
  } hilo;
  CacheLine iCache[256];
  union {
    uint32_t value;
    struct packed __CpuCacheControlReg {
      uint32_t unused0_1 : 2;
      uint32_t tagTestMode : 1;
      uint32_t scratchpadEnabled1 : 1;
      uint32_t unused4_6 : 3;
      uint32_t scratchpadEnabled2 : 1;
      uint32_t unused8_10 : 3;
      uint32_t codeCacheEnabled : 1;
      uint32_t unused12_31 : 20;
    } parsed;
  } cacheControlReg;
  CpuCop0 cop0;
  Clock *clock;
  uint64_t cycles;
};

typedef void (*_Nullable OpcodeHandler)(Cpu *cpu, Instruction instruction);

static inline Instruction NewInstruction(uint32_t value) {
  Instruction instruction = {.value = value};
  return instruction;
}

ASSUME_NONNULL_END
