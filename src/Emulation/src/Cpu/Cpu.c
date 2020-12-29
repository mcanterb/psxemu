#include "Cpu.h"
#include "../Clock.h"
#include "../System.h"
#include "../Types.h"
#include "Instructions.h"

ASSUME_NONNULL_BEGIN

static OpcodeHandler kOpcodeTable[64] = {
    Rtype, Bcond,  J,    Jal,    Beq, Bne, Blez, Bgtz, Addi, Addiu,  Slti, Sltiu,  Andi, Ori, Xori, Lui,
    Cop0,  UnkCop, Cop2, UnkCop, Unk, Unk, Unk,  Unk,  Unk,  Unk,    Unk,  Unk,    Unk,  Unk, Unk,  Unk,
    Lb,    Lh,     Lwl,  Lw,     Lbu, Lhu, Lwr,  Unk,  Sb,   Sh,     Swl,  Sw,     Unk,  Unk, Swr,  Unk,
    Lwc0,  UnkCop, Lwc2, UnkCop, Unk, Unk, Unk,  Unk,  Swc0, UnkCop, Swc2, UnkCop, Unk,  Unk, Unk,  Unk};

static OpcodeHandler kRegisterFunctTable[64] = {
    Sll,  Unk,  Srl,  Sra,  Sllv, Unk, Srlv, Srav, Jr,   Jalr,  Unk, Unk,  Syscall, Break, Unk, Unk,
    Mfhi, Mthi, Mflo, Mtlo, Unk,  Unk, Unk,  Unk,  Mult, Multu, Div, Divu, Unk,     Unk,   Unk, Unk,
    Add,  Addu, Sub,  Subu, And,  Or,  Xor,  Nor,  Unk,  Unk,   Slt, Sltu, Unk,     Unk,   Unk, Unk,
    Unk,  Unk,  Unk,  Unk,  Unk,  Unk, Unk,  Unk,  Unk,  Unk,   Unk, Unk,  Unk,     Unk,   Unk, Unk};

static void RunNextInstruction(Cpu *cpu);
static void Store32(Cpu *cpu, Address address, uint32_t value);
static void Store16(Cpu *cpu, Address address, uint16_t value);
static void Store8(Cpu *cpu, Address address, uint8_t value);
static bool Load32(Cpu *cpu, Address address, uint32_t *result);
static bool Load16(Cpu *cpu, Address address, uint16_t *result);
static bool Load8(Cpu *cpu, Address address, uint8_t *result);
static bool LoadNextInstruction(Cpu *cpu, Instruction *result);
static Address ExceptionHandler(Cpu *cpu, SystemException exception);
static void Exception(Cpu *cpu, SystemException exception);
static void CacheControlWrite32(Cpu *cpu, Address address, MemorySegment segment, uint32_t value);
static void CacheControlWrite16(Cpu *cpu, Address address, MemorySegment segment, uint16_t value);
static void CacheControlWrite8(Cpu *cpu, Address address, MemorySegment segment, uint8_t value);
static uint32_t CacheControlRead32(Cpu *cpu, Address address, MemorySegment segment);
static uint16_t CacheControlRead16(Cpu *cpu, Address address, MemorySegment segment);
static uint8_t CacheControlRead8(Cpu *cpu, Address address, MemorySegment segment);
static void CacheMaintenance(Cpu *cpu, Address address, uint32_t value);
static void DecodeAndExecute(Cpu *cpu, Instruction instruction);
static void CpuDelayedLoad(Cpu *cpu);
static void CpuDelayedLoadAndSetLoad(Cpu *cpu, uint8_t reg, uint32_t value);
static void CpuJumpToAddress(Cpu *cpu, Address address);
static void CpuLink(Cpu *cpu, uint8_t linkReg);

Cpu *CpuNew(System *sys, Bus *bus, Clock *clock) {
  Cpu *cpu = (Cpu *)SystemArenaAllocate(sys, sizeof(Cpu));
  cpu->bus = bus;
  int i;
  for (i = 1; i < 32; i++) {
    cpu->reg[i] = 0xDEADBEEF;
  }
  cpu->reg[0] = 0;
  cpu->pc = kResetVector;
  cpu->nextPc = cpu->pc + 4;
  cpu->currentPc = 0;
  cpu->loadReg = 0;
  cpu->loadValue = 0;
  cpu->clock = clock;

  cpu->cop0.badVaddr = 0;
  cpu->cop0.cause.value = 0;
  cpu->cop0.epc = 0;
  cpu->cop0.sr.value = 0;

  return cpu;
}

void CpuRegisterCacheControl(Cpu *cpu) {
  BusDevice device = {.context = cpu,
                      .cpuCycles = 0,
                      .read32 = (Read32)CacheControlRead32,
                      .read16 = (Read16)CacheControlRead16,
                      .read8 = (Read8)CacheControlRead8,
                      .write32 = (Write32)CacheControlWrite32,
                      .write16 = (Write16)CacheControlWrite16,
                      .write8 = (Write8)CacheControlWrite8};
  PCFResultOrPanic(BusRegisterDevice(cpu->bus, &device, NewAddressRange(0xFFFE0130, 0xFFFE0134, KernelSegment2)));
}

void CpuRun(Cpu *cpu, uint32_t cycles) {

  uint32_t numCycles = 0;
  while (cycles > numCycles) {
    RunNextInstruction(cpu);
    ClockTick(cpu->clock, cpu->cycles);
    numCycles += cpu->cycles;
    cpu->cycles = 0;
  }
}

static void RunNextInstruction(Cpu *cpu) {
  cpu->currentPc = cpu->pc;
  Instruction instruction;
  if (!LoadNextInstruction(cpu, &instruction)) {
    return;
  }
  cpu->pc = cpu->nextPc;
  cpu->nextPc = cpu->pc + 4;
  cpu->delaySlot = cpu->branch;
  cpu->branch = false;
  // TODO: Check for interrupts

  DecodeAndExecute(cpu, instruction);
  cpu->reg[0] = 0;
}

static void Exception(Cpu *cpu, SystemException exception) { PCF_PANIC("System Exceptions are not implemented yet!"); }

static bool LoadNextInstruction(Cpu *cpu, Instruction *result) {
  Address address = cpu->pc;
  MemorySegment segment = MemorySegmentForAddress(address);
  if ((segment == UserSegment || segment == KernelSegment0) && cpu->cacheControlReg.parsed.codeCacheEnabled) {
    // Load From Code Cache
    size_t lineNumber = (address >> 4) & 0xFF;
    CacheLine *line = &cpu->iCache[lineNumber];
    // TODO: Finish code cache implementation
  }
  uint32_t loadResult;
  if (Load32(cpu, address, &loadResult)) {
    *result = NewInstruction(loadResult);
    return true;
  }
  return false;
}

static void DecodeAndExecute(Cpu *cpu, Instruction instruction) {
  kOpcodeTable[instruction.imm.op](cpu, instruction);
  cpu->cycles++;
}

static void Unk(Cpu *cpu, Instruction instruction) {
  PCF_PANIC("Unimplemented instruction 0x%08x", instruction);
  CpuDelayedLoad(cpu);
}
static void UnkCop(Cpu *cpu, Instruction instruction) {
  PCF_PANIC("Unimplemented instruction targeting unknown coprocessor 0x%08x", instruction);
  CpuDelayedLoad(cpu);
}
static void Rtype(Cpu *cpu, Instruction instruction) { kRegisterFunctTable[instruction.reg.funct](cpu, instruction); }
static void Bcond(Cpu *cpu, Instruction instruction) {
  uint8_t code = instruction.imm.rt;
  switch (code) {
  case 0x00:
    Bltz(cpu, instruction);
    break;
  case 0x10:
    Bltzal(cpu, instruction);
  case 0x01:
    Bgez(cpu, instruction);
    break;
  case 0x11:
    Bgezal(cpu, instruction);
    break;
  default:
    Unk(cpu, instruction);
  }
}

static void J(Cpu *cpu, Instruction instruction) {
  CpuDelayedLoad(cpu);
  Address address = instruction.jump.target << 2;
  address = address | ((cpu->pc) & 0xF0000000);
  cpu->branch = true;
  CpuJumpToAddress(cpu, address);
}

static void Jal(Cpu *cpu, Instruction instruction) {
  Address address = instruction.jump.target << 2;
  address = address | ((cpu->pc) & 0xF0000000);
  cpu->branch = true;
  CpuJumpToAddress(cpu, address);
  CpuLink(cpu, 31);
}

static void Beq(Cpu *cpu, Instruction instruction) {
  uint32_t rs = cpu->reg[instruction.imm.rs];
  uint32_t rt = cpu->reg[instruction.imm.rt];
  CpuDelayedLoad(cpu);
  cpu->branch = true;
  if (rs == rt) {
    int32_t offset = SIGN_EXTEND(instruction.imm.immediate) << 2;
    Address address = cpu->pc + offset;
    CpuJumpToAddress(cpu, address);
  }
}
static void Bne(Cpu *cpu, Instruction instruction) {
  uint32_t rs = cpu->reg[instruction.imm.rs];
  uint32_t rt = cpu->reg[instruction.imm.rt];
  CpuDelayedLoad(cpu);
  cpu->branch = true;
  if (rs != rt) {
    int32_t offset = SIGN_EXTEND(instruction.imm.immediate) << 2;
    Address address = cpu->pc + offset;
    CpuJumpToAddress(cpu, address);
  }
}

static void Blez(Cpu *cpu, Instruction instruction) {
  int32_t rs = cpu->reg[instruction.imm.rs];
  CpuDelayedLoad(cpu);
  cpu->branch = true;
  if (rs <= 0) {
    int32_t offset = SIGN_EXTEND(instruction.imm.immediate) << 2;
    Address address = cpu->pc + offset;
    CpuJumpToAddress(cpu, address);
  }
}

static void Bgtz(Cpu *cpu, Instruction instruction) {
  int32_t rs = cpu->reg[instruction.imm.rs];
  CpuDelayedLoad(cpu);
  cpu->branch = true;
  if (rs > 0) {
    int32_t offset = SIGN_EXTEND(instruction.imm.immediate) << 2;
    Address address = cpu->pc + offset;
    CpuJumpToAddress(cpu, address);
  }
}

static void Addi(Cpu *cpu, Instruction instruction) {
  int32_t rs = cpu->reg[instruction.imm.rs];
  int32_t imm = SIGN_EXTEND(instruction.imm.immediate);
  CpuDelayedLoad(cpu);
  int32_t result;
  if (!__builtin_sadd_overflow(rs, imm, &result)) {
    cpu->reg[instruction.imm.rt] = result;
  } else {
    Exception(cpu, NewSystemException(kExceptionOverflow, cpu->pc));
  }
}

static void Addiu(Cpu *cpu, Instruction instruction) {
  uint32_t rs = cpu->reg[instruction.imm.rs];
  uint32_t imm = SIGN_EXTEND(instruction.imm.immediate);
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.imm.rt] = rs + imm;
}

static void Slti(Cpu *cpu, Instruction instruction) {
  int32_t rs = cpu->reg[instruction.imm.rs];
  int32_t imm = SIGN_EXTEND(instruction.imm.immediate);
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.imm.rt] = (uint32_t)((bool)(rs < imm));
}

static void Sltiu(Cpu *cpu, Instruction instruction) {
  uint32_t rs = cpu->reg[instruction.imm.rs];
  uint32_t imm = SIGN_EXTEND(instruction.imm.immediate);
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.imm.rt] = (uint32_t)((bool)(rs < imm));
}

static void Andi(Cpu *cpu, Instruction instruction) {
  uint32_t rs = cpu->reg[instruction.imm.rs];
  uint32_t imm = instruction.imm.immediate;
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.imm.rt] = rs & imm;
}

static void Ori(Cpu *cpu, Instruction instruction) {
  uint32_t rs = cpu->reg[instruction.imm.rs];
  uint32_t imm = instruction.imm.immediate;
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.imm.rt] = rs | imm;
}

static void Xori(Cpu *cpu, Instruction instruction) {
  uint32_t rs = cpu->reg[instruction.imm.rs];
  uint32_t imm = instruction.imm.immediate;
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.imm.rt] = rs ^ imm;
}

static void Lui(Cpu *cpu, Instruction instruction) {
  uint32_t imm = instruction.imm.immediate;
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.imm.rt] = (imm << 16);
}

static void Cop0(Cpu *cpu, Instruction instruction) {
  if (instruction.copCommand.unused25) {
    // Cop Command
    if (instruction.copCommand.command == 0x10) {
      Rfe(cpu, instruction);
      return;
    }
    PCFERROR("Unknown Cop0 command : 0x%08x", instruction.copCommand.command);
    Unk(cpu, instruction);
  }
  switch (instruction.copReg.subOpcode) {
  case 0:
    Mfc0(cpu, instruction);
    return;
  case 4:
    Mtc0(cpu, instruction);
    return;
  }
  Unk(cpu, instruction);
}

static void Mtc0(Cpu *cpu, Instruction instruction) {
  uint32_t rt = cpu->reg[instruction.copReg.rt];
  CpuDelayedLoad(cpu);
  switch (instruction.copReg.rdCop) {
  case 3:
  case 5:
  case 6:
  case 7:
  case 9:
  case 11:
    if (rt != 0) {
      PCF_PANIC("Unsupported write to breakpoint registers! cop0 reg = %d, value = 0x%08x", instruction.copReg.rdCop,
                rt);
    }
    return;
  case 12:
    cpu->cop0.sr.value = rt;
    return;
  case 13:
    cpu->cop0.cause.value = rt;
    return;
  }
  PCF_PANIC("Write to Unsupported COP0 reg: %d", instruction.copReg.rdCop);
}

static void Mfc0(Cpu *cpu, Instruction instruction) {
  uint32_t value;
  switch (instruction.copReg.rdCop) {
  case 12:
    value = cpu->cop0.sr.value;
    break;
  case 13:
    value = cpu->cop0.cause.value;
    break;
  case 14:
    value = cpu->cop0.epc;
    break;
  case 15:
    value = kProcessorId;
    break;
  default:
    PCFWARN("Read from Unsupported COP0 reg: %d", instruction.copReg.rdCop);
    value = 0;
  }
  CpuDelayedLoadAndSetLoad(cpu, instruction.copReg.rt, value);
}

static void Cop2(Cpu *cpu, Instruction instruction) {
  PCFWARN("GTE (COP2) instructions are unimplemented!");
  CpuDelayedLoad(cpu);
}

static void Lb(Cpu *cpu, Instruction instruction) {
  uint32_t rs = cpu->reg[instruction.imm.rs];
  uint32_t offset = SIGN_EXTEND(instruction.imm.immediate);
  uint8_t result;
  if (Load8(cpu, rs + offset, &result)) {
    CpuDelayedLoadAndSetLoad(cpu, instruction.imm.rt, SIGN_EXTEND(result));
    return;
  }
  CpuDelayedLoad(cpu);
}

static void Lh(Cpu *cpu, Instruction instruction) {
  uint32_t rs = cpu->reg[instruction.imm.rs];
  uint32_t offset = SIGN_EXTEND(instruction.imm.immediate);
  uint16_t result;
  if (Load16(cpu, rs + offset, &result)) {
    CpuDelayedLoadAndSetLoad(cpu, instruction.imm.rt, SIGN_EXTEND(result));
    return;
  }
  CpuDelayedLoad(cpu);
}

static void Lwl(Cpu *cpu, Instruction instruction) {
  uint32_t rs = cpu->reg[instruction.imm.rs];
  uint32_t offset = SIGN_EXTEND(instruction.imm.immediate);
  Address alignedAddress = (rs + offset) & 0xFFFFFFFC;
  uint32_t memValue;
  if (Load32(cpu, alignedAddress, &memValue)) {
    uint32_t index = (rs + offset) & 0x00000003;
    uint32_t value;
    if (cpu->loadReg == instruction.imm.rt) {
      value = cpu->loadValue;
    } else {
      value = cpu->reg[instruction.imm.rt];
    }
    switch (index) {
    case 0:
      value = (value & 0x00FFFFFF) | (memValue << 24);
      break;
    case 1:
      value = (value & 0x0000FFFF) | (memValue << 16);
      break;
    case 2:
      value = (value & 0x000000FF) | (memValue << 8);
      break;
    case 3:
      value = memValue;
      break;
    }
    CpuDelayedLoadAndSetLoad(cpu, instruction.imm.rt, value);
  } else {
    CpuDelayedLoad(cpu);
  }
}

static void Lw(Cpu *cpu, Instruction instruction) {
  uint32_t rs = cpu->reg[instruction.imm.rs];
  uint32_t offset = SIGN_EXTEND(instruction.imm.immediate);
  uint32_t result;
  if (Load32(cpu, rs + offset, &result)) {
    CpuDelayedLoadAndSetLoad(cpu, instruction.imm.rt, result);
    return;
  }
  CpuDelayedLoad(cpu);
}

static void Lbu(Cpu *cpu, Instruction instruction) {
  uint32_t rs = cpu->reg[instruction.imm.rs];
  uint32_t offset = SIGN_EXTEND(instruction.imm.immediate);
  uint8_t result;
  if (Load8(cpu, rs + offset, &result)) {
    CpuDelayedLoadAndSetLoad(cpu, instruction.imm.rt, (uint32_t)result);
    return;
  }
  CpuDelayedLoad(cpu);
}

static void Lhu(Cpu *cpu, Instruction instruction) {
  uint32_t rs = cpu->reg[instruction.imm.rs];
  uint32_t offset = SIGN_EXTEND(instruction.imm.immediate);
  uint16_t result;
  if (Load16(cpu, rs + offset, &result)) {
    CpuDelayedLoadAndSetLoad(cpu, instruction.imm.rt, (uint32_t)result);
    return;
  }
  CpuDelayedLoad(cpu);
}

static void Lwr(Cpu *cpu, Instruction instruction) {
  uint32_t rs = cpu->reg[instruction.imm.rs];
  uint32_t offset = SIGN_EXTEND(instruction.imm.immediate);
  Address alignedAddress = (rs + offset) & 0xFFFFFFFC;
  uint32_t memValue;
  if (Load32(cpu, alignedAddress, &memValue)) {
    uint32_t index = (rs + offset) & 0x00000003;
    uint32_t value;
    if (cpu->loadReg == instruction.imm.rt) {
      value = cpu->loadValue;
    } else {
      value = cpu->reg[instruction.imm.rt];
    }
    switch (index) {
    case 0:
      value = memValue;
      break;
    case 1:
      value = (value & 0xFF000000) | (memValue >> 8);
      break;
    case 2:
      value = (value & 0xFFFF0000) | (memValue >> 16);
      break;
    case 3:
      value = (value & 0xFFFFFF00) | (memValue >> 24);
      break;
    }
    CpuDelayedLoadAndSetLoad(cpu, instruction.imm.rt, value);
  } else {
    CpuDelayedLoad(cpu);
  }
}

static void Sb(Cpu *cpu, Instruction instruction) {
  uint32_t rs = cpu->reg[instruction.imm.rs];
  uint32_t offset = SIGN_EXTEND(instruction.imm.immediate);
  Store8(cpu, (rs + offset), cpu->reg[instruction.imm.rt]);
  CpuDelayedLoad(cpu);
}

static void Sh(Cpu *cpu, Instruction instruction) {
  uint32_t rs = cpu->reg[instruction.imm.rs];
  uint32_t offset = SIGN_EXTEND(instruction.imm.immediate);
  Store16(cpu, (rs + offset), cpu->reg[instruction.imm.rt]);
  CpuDelayedLoad(cpu);
}

static void Swl(Cpu *cpu, Instruction instruction) {
  uint32_t rs = cpu->reg[instruction.imm.rs];
  uint32_t offset = SIGN_EXTEND(instruction.imm.immediate);
  uint32_t rt = cpu->reg[instruction.imm.rt];
  Address alignedAddress = (rs + offset) & 0xFFFFFFFC;
  CpuDelayedLoad(cpu);
  uint32_t value;
  if (Load32(cpu, alignedAddress, &value)) {
    uint32_t index = (rs + offset) & 0x00000003;

    switch (index) {
    case 0:
      value = (value & 0xFFFFFF00) | (rt >> 24);
      break;
    case 1:
      value = (value & 0xFFFF0000) | (rt >> 16);
      break;
    case 2:
      value = (value & 0xFF000000) | (rt >> 8);
      break;
    case 3:
      value = rt;
      break;
    }
    Store32(cpu, alignedAddress, value);
  }
}

static void Sw(Cpu *cpu, Instruction instruction) {
  uint32_t rs = cpu->reg[instruction.imm.rs];
  uint32_t offset = SIGN_EXTEND(instruction.imm.immediate);
  Store32(cpu, (rs + offset), cpu->reg[instruction.imm.rt]);
  CpuDelayedLoad(cpu);
}

static void Swr(Cpu *cpu, Instruction instruction) {
  uint32_t rs = cpu->reg[instruction.imm.rs];
  uint32_t offset = SIGN_EXTEND(instruction.imm.immediate);
  uint32_t rt = cpu->reg[instruction.imm.rt];
  Address alignedAddress = (rs + offset) & 0xFFFFFFFC;
  CpuDelayedLoad(cpu);
  uint32_t value;
  if (Load32(cpu, alignedAddress, &value)) {
    uint32_t index = (rs + offset) & 0x00000003;

    switch (index) {
    case 0:
      value = rt;
      break;
    case 1:
      value = (value & 0x000000FF) | (rt << 8);
      break;
    case 2:
      value = (value & 0x0000FFFF) | (rt << 16);
      break;
    case 3:
      value = (value & 0x00FFFFFF) | (rt << 24);
      break;
    }
    Store32(cpu, alignedAddress, value);
  }
}

static void Lwc0(Cpu *cpu, Instruction instruction) { Unk(cpu, instruction); }

static void Lwc2(Cpu *cpu, Instruction instruction) {
  PCFWARN("GTE (COP2) instructions are unimplemented!");
  CpuDelayedLoad(cpu);
}

static void Swc0(Cpu *cpu, Instruction instruction) { Unk(cpu, instruction); }

static void Swc2(Cpu *cpu, Instruction instruction) {
  PCFWARN("GTE (COP2) instructions are unimplemented!");
  CpuDelayedLoad(cpu);
}

static void Sll(Cpu *cpu, Instruction instruction) {
  uint32_t rt = cpu->reg[instruction.reg.rt];
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.reg.rd] = rt << instruction.reg.shamt;
}

static void Srl(Cpu *cpu, Instruction instruction) {
  uint32_t rt = cpu->reg[instruction.reg.rt];
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.reg.rd] = rt >> instruction.reg.shamt;
}

static void Sra(Cpu *cpu, Instruction instruction) {
  int32_t rt = cpu->reg[instruction.reg.rt];
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.reg.rd] = rt >> instruction.reg.shamt;
}

static void Sllv(Cpu *cpu, Instruction instruction) {
  uint32_t rt = cpu->reg[instruction.reg.rt];
  uint8_t shamt = cpu->reg[instruction.reg.rs] & 0x0000001F;
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.reg.rd] = rt << shamt;
}

static void Srlv(Cpu *cpu, Instruction instruction) {
  uint32_t rt = cpu->reg[instruction.reg.rt];
  uint8_t shamt = cpu->reg[instruction.reg.rs] & 0x0000001F;
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.reg.rd] = rt >> shamt;
}

static void Srav(Cpu *cpu, Instruction instruction) {
  int32_t rt = cpu->reg[instruction.reg.rt];
  uint8_t shamt = cpu->reg[instruction.reg.rs] & 0x0000001F;
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.reg.rd] = rt >> shamt;
}

static void Jr(Cpu *cpu, Instruction instruction) {
  Address address = cpu->reg[instruction.reg.rs];
  cpu->branch = true;
  CpuDelayedLoad(cpu);
  CpuJumpToAddress(cpu, address);
}

static void Jalr(Cpu *cpu, Instruction instruction) {
  Address address = cpu->reg[instruction.reg.rs];
  cpu->branch = true;
  CpuJumpToAddress(cpu, address);
  CpuLink(cpu, instruction.reg.rd);
}

static void Syscall(Cpu *cpu, Instruction instruction) {
  CpuDelayedLoad(cpu);
  Exception(cpu, NewSystemException(kExceptionSyscall, cpu->pc));
}

static void Break(Cpu *cpu, Instruction instruction) {
  CpuDelayedLoad(cpu);
  Exception(cpu, NewSystemException(kExceptionBreakPoint, cpu->pc));
}

static void Mfhi(Cpu *cpu, Instruction instruction) {
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.reg.rd] = cpu->hilo.distinct.hi;
}

static void Mthi(Cpu *cpu, Instruction instruction) {
  cpu->hilo.distinct.hi = cpu->reg[instruction.reg.rs];
  CpuDelayedLoad(cpu);
}

static void Mflo(Cpu *cpu, Instruction instruction) {
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.reg.rd] = cpu->hilo.distinct.lo;
}

static void Mtlo(Cpu *cpu, Instruction instruction) {
  cpu->hilo.distinct.lo = cpu->reg[instruction.reg.rs];
  CpuDelayedLoad(cpu);
}

static void Mult(Cpu *cpu, Instruction instruction) {
  int64_t rs = cpu->reg[instruction.reg.rs];
  int64_t rt = cpu->reg[instruction.reg.rt];
  CpuDelayedLoad(cpu);
  cpu->hilo.combined = rs * rt;
}

static void Multu(Cpu *cpu, Instruction instruction) {
  uint64_t rs = cpu->reg[instruction.reg.rs];
  uint64_t rt = cpu->reg[instruction.reg.rt];
  CpuDelayedLoad(cpu);
  cpu->hilo.combined = rs * rt;
}

static void Div(Cpu *cpu, Instruction instruction) {
  int64_t rs = cpu->reg[instruction.reg.rs];
  int64_t rt = cpu->reg[instruction.reg.rt];
  CpuDelayedLoad(cpu);
  if (rt == 0) {
    cpu->hilo.distinct.hi = rs;
    if (rs >= 0) {
      cpu->hilo.distinct.lo = 0xFFFFFFFF;
    } else {
      cpu->hilo.distinct.lo = 1;
    }
  } else if (rs == 0x80000000 && rt == -1) {
    cpu->hilo.distinct.hi = 0;
    cpu->hilo.distinct.lo = 0x80000000;
  } else {
    cpu->hilo.distinct.lo = rs / rt;
    cpu->hilo.distinct.hi = rs % rt;
  }
}

static void Divu(Cpu *cpu, Instruction instruction) {
  uint64_t rs = cpu->reg[instruction.reg.rs];
  uint64_t rt = cpu->reg[instruction.reg.rt];
  CpuDelayedLoad(cpu);
  if (rt == 0) {
    cpu->hilo.distinct.hi = rs;
    if (rs >= 0) {
      cpu->hilo.distinct.lo = 0xFFFFFFFF;
    } else {
      cpu->hilo.distinct.lo = 1;
    }
  } else {
    cpu->hilo.distinct.lo = rs / rt;
    cpu->hilo.distinct.hi = rs % rt;
  }
}

static void Add(Cpu *cpu, Instruction instruction) {
  int32_t rs = cpu->reg[instruction.reg.rs];
  int32_t rt = cpu->reg[instruction.reg.rt];
  CpuDelayedLoad(cpu);
  int32_t result;
  if (!__builtin_sadd_overflow(rs, rt, &result)) {
    cpu->reg[instruction.reg.rd] = result;
  } else {
    Exception(cpu, NewSystemException(kExceptionOverflow, cpu->pc));
  }
}

static void Addu(Cpu *cpu, Instruction instruction) {
  int32_t rs = cpu->reg[instruction.reg.rs];
  int32_t rt = cpu->reg[instruction.reg.rt];
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.reg.rd] = rs + rt;
}

static void Sub(Cpu *cpu, Instruction instruction) {
  int32_t rs = cpu->reg[instruction.reg.rs];
  int32_t rt = cpu->reg[instruction.reg.rt];
  CpuDelayedLoad(cpu);
  int32_t result;
  if (!__builtin_sadd_overflow(rs, -rt, &result)) {
    cpu->reg[instruction.reg.rd] = result;
  } else {
    Exception(cpu, NewSystemException(kExceptionOverflow, cpu->pc));
  }
}

static void Subu(Cpu *cpu, Instruction instruction) {
  int32_t rs = cpu->reg[instruction.reg.rs];
  int32_t rt = cpu->reg[instruction.reg.rt];
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.reg.rd] = rs - rt;
}

static void And(Cpu *cpu, Instruction instruction) {
  int32_t rs = cpu->reg[instruction.reg.rs];
  int32_t rt = cpu->reg[instruction.reg.rt];
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.reg.rd] = rs & rt;
}

static void Or(Cpu *cpu, Instruction instruction) {
  int32_t rs = cpu->reg[instruction.reg.rs];
  int32_t rt = cpu->reg[instruction.reg.rt];
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.reg.rd] = rs | rt;
}

static void Xor(Cpu *cpu, Instruction instruction) {
  int32_t rs = cpu->reg[instruction.reg.rs];
  int32_t rt = cpu->reg[instruction.reg.rt];
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.reg.rd] = rs ^ rt;
}

static void Nor(Cpu *cpu, Instruction instruction) {
  int32_t rs = cpu->reg[instruction.reg.rs];
  int32_t rt = cpu->reg[instruction.reg.rt];
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.reg.rd] = !(rs | rt);
}

static void Slt(Cpu *cpu, Instruction instruction) {
  int32_t rs = cpu->reg[instruction.reg.rs];
  int32_t rt = cpu->reg[instruction.reg.rt];
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.reg.rd] = (uint32_t)((bool)(rs < rt));
}

static void Sltu(Cpu *cpu, Instruction instruction) {
  uint32_t rs = cpu->reg[instruction.reg.rs];
  uint32_t rt = cpu->reg[instruction.reg.rt];
  CpuDelayedLoad(cpu);
  cpu->reg[instruction.reg.rd] = (uint32_t)((bool)(rs < rt));
}

static void Bltz(Cpu *cpu, Instruction instruction) {
  int32_t rs = cpu->reg[instruction.imm.rs];
  CpuDelayedLoad(cpu);
  cpu->branch = true;
  if (rs < 0) {
    int32_t offset = SIGN_EXTEND(instruction.imm.immediate) << 2;
    Address address = cpu->pc + offset;
    CpuJumpToAddress(cpu, address);
  }
}

static void Bgez(Cpu *cpu, Instruction instruction) {
  int32_t rs = cpu->reg[instruction.imm.rs];
  CpuDelayedLoad(cpu);
  cpu->branch = true;
  if (rs >= 0) {
    int32_t offset = SIGN_EXTEND(instruction.imm.immediate) << 2;
    Address address = cpu->pc + offset;
    CpuJumpToAddress(cpu, address);
  }
}

static void Bltzal(Cpu *cpu, Instruction instruction) {
  int32_t rs = cpu->reg[instruction.imm.rs];
  cpu->branch = true;
  if (rs < 0) {
    int32_t offset = SIGN_EXTEND(instruction.imm.immediate) << 2;
    Address address = cpu->pc + offset;
    CpuJumpToAddress(cpu, address);
    CpuLink(cpu, 31);
  }
}

static void Bgezal(Cpu *cpu, Instruction instruction) {
  int32_t rs = cpu->reg[instruction.imm.rs];
  cpu->branch = true;
  if (rs >= 0) {
    int32_t offset = SIGN_EXTEND(instruction.imm.immediate) << 2;
    Address address = cpu->pc + offset;
    CpuJumpToAddress(cpu, address);
    CpuLink(cpu, 31);
  }
}

static void Rfe(Cpu *cpu, Instruction instruction) { CpuDelayedLoad(cpu); }

static void CpuDelayedLoad(Cpu *cpu) {
  cpu->reg[cpu->loadReg] = cpu->loadValue;
  cpu->loadReg = 0;
  cpu->loadValue = 0;
}

static void CpuDelayedLoadAndSetLoad(Cpu *cpu, uint8_t reg, uint32_t value) {
  uint8_t pendingReg = cpu->loadReg;
  if (pendingReg != reg) {
    cpu->reg[pendingReg] = cpu->loadValue;
  }
  cpu->loadReg = reg;
  cpu->loadValue = value;
}

static void CpuJumpToAddress(Cpu *cpu, Address address) {
  cpu->nextPc = address;
  cpu->branch = true;
}
static void CpuLink(Cpu *cpu, uint8_t linkReg) {
  Address returnAddress = cpu->currentPc + 8;
  CpuDelayedLoadAndSetLoad(cpu, linkReg, returnAddress);
}

static void Store32(Cpu *cpu, Address address, uint32_t value) {
  if (cpu->cop0.sr.parsed.cacheIsolated) {
    CacheMaintenance(cpu, address, value);
    return;
  }
  SystemException exception;
  uint32_t cycles;
  if (!BusWrite32(cpu->bus, address, value, &exception, &cycles)) {
    Exception(cpu, exception);
    return;
  }
}

static void Store16(Cpu *cpu, Address address, uint16_t value) {
  if (cpu->cop0.sr.parsed.cacheIsolated) {
    PCF_PANIC("Unsupported Store16 while Cache is Isolated! Address = " ADDR_FORMAT ", value = 0x%04x", address, value);
    return;
  }
  SystemException exception;
  uint32_t cycles;
  if (!BusWrite16(cpu->bus, address, value, &exception, &cycles)) {
    Exception(cpu, exception);
    return;
  }
}

static void Store8(Cpu *cpu, Address address, uint8_t value) {
  if (cpu->cop0.sr.parsed.cacheIsolated) {
    PCF_PANIC("Unsupported Store8 while Cache is Isolated! Address = " ADDR_FORMAT ", value = 0x%02x", address, value);
    return;
  }
  SystemException exception;
  uint32_t cycles;
  if (!BusWrite8(cpu->bus, address, value, &exception, &cycles)) {
    Exception(cpu, exception);
    return;
  }
}

static bool Load32(Cpu *cpu, Address address, uint32_t *result) {
  SystemException exception;
  uint32_t cycles;
  BusRead32(cpu->bus, address, result, &exception, &cycles);
  if (!BusRead32(cpu->bus, address, result, &exception, &cycles)) {
    Exception(cpu, exception);
    return false;
  }
  cpu->cycles += cycles;
  return true;
}

static bool Load16(Cpu *cpu, Address address, uint16_t *result) {
  SystemException exception;
  uint32_t cycles;
  BusRead16(cpu->bus, address, result, &exception, &cycles);
  if (!BusRead16(cpu->bus, address, result, &exception, &cycles)) {
    Exception(cpu, exception);
    return false;
  }
  cpu->cycles += cycles;
  return true;
}

static bool Load8(Cpu *cpu, Address address, uint8_t *result) {
  SystemException exception;
  uint32_t cycles;
  BusRead8(cpu->bus, address, result, &exception, &cycles);
  if (!BusRead8(cpu->bus, address, result, &exception, &cycles)) {
    Exception(cpu, exception);
    return false;
  }
  cpu->cycles += cycles;
  return true;
}

static void CacheControlWrite32(Cpu *cpu, Address address, MemorySegment segment, uint32_t value) {
  cpu->cacheControlReg.value = value;
}

static void CacheControlWrite16(Cpu *cpu, Address address, MemorySegment segment, uint16_t value) {
  PCF_PANIC("Attempted to write 16-bit value to CacheControl! Value = 0x%04x", value);
}

static void CacheControlWrite8(Cpu *cpu, Address address, MemorySegment segment, uint8_t value) {
  PCF_PANIC("Attempted to write 8-bit value to CacheControl! Value = 0x%02x", value);
}

static uint32_t CacheControlRead32(Cpu *cpu, Address address, MemorySegment segment) {
  return cpu->cacheControlReg.value;
}

static uint16_t CacheControlRead16(Cpu *cpu, Address address, MemorySegment segment) {
  PCF_PANIC("Attempted to read 16-bit value from CacheControl");
  return 0;
}

static uint8_t CacheControlRead8(Cpu *cpu, Address address, MemorySegment segment) {
  PCF_PANIC("Attempted to read 8-bit value from CacheControl");
  return 0;
}

static void CacheMaintenance(Cpu *cpu, Address address, uint32_t value) {
  if (!cpu->cacheControlReg.parsed.codeCacheEnabled) {
    PCF_PANIC("Attempting cache maintenance while code cache is disabled!");
    return;
  }
  size_t lineNumber = (address >> 4) & 0xFF;
  CacheLine *line = &cpu->iCache[lineNumber];
  if (cpu->cacheControlReg.parsed.tagTestMode) {
    line->tagValid.parsed.isValid = false;
  } else {
    size_t index = (address >> 2) & 0x03;
    line->entries[index] = NewInstruction(value);
  }
}

ASSUME_NONNULL_END
