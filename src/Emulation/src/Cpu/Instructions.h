#pragma once
#include "../Types.h"
#include "Types.h"

ASSUME_NONNULL_BEGIN

static void Unk(Cpu *cpu, Instruction instruction);
static void UnkCop(Cpu *cpu, Instruction instruction);
static void Rtype(Cpu *cpu, Instruction instruction);
static void Bcond(Cpu *cpu, Instruction instruction);
static void J(Cpu *cpu, Instruction instruction);
static void Jal(Cpu *cpu, Instruction instruction);
static void Beq(Cpu *cpu, Instruction instruction);
static void Bne(Cpu *cpu, Instruction instruction);
static void Blez(Cpu *cpu, Instruction instruction);
static void Bgtz(Cpu *cpu, Instruction instruction);
static void Addi(Cpu *cpu, Instruction instruction);
static void Addiu(Cpu *cpu, Instruction instruction);
static void Slti(Cpu *cpu, Instruction instruction);
static void Sltiu(Cpu *cpu, Instruction instruction);
static void Andi(Cpu *cpu, Instruction instruction);
static void Ori(Cpu *cpu, Instruction instruction);
static void Xori(Cpu *cpu, Instruction instruction);
static void Lui(Cpu *cpu, Instruction instruction);
static void Cop0(Cpu *cpu, Instruction instruction);
static void Mtc0(Cpu *cpu, Instruction instruction);
static void Mfc0(Cpu *cpu, Instruction instruction);
static void Cop2(Cpu *cpu, Instruction instruction);
static void Lb(Cpu *cpu, Instruction instruction);
static void Lh(Cpu *cpu, Instruction instruction);
static void Lwl(Cpu *cpu, Instruction instruction);
static void Lw(Cpu *cpu, Instruction instruction);
static void Lbu(Cpu *cpu, Instruction instruction);
static void Lhu(Cpu *cpu, Instruction instruction);
static void Lwr(Cpu *cpu, Instruction instruction);
static void Sb(Cpu *cpu, Instruction instruction);
static void Sh(Cpu *cpu, Instruction instruction);
static void Swl(Cpu *cpu, Instruction instruction);
static void Sw(Cpu *cpu, Instruction instruction);
static void Swr(Cpu *cpu, Instruction instruction);
static void Lwc0(Cpu *cpu, Instruction instruction);
static void Lwc2(Cpu *cpu, Instruction instruction);
static void Swc0(Cpu *cpu, Instruction instruction);
static void Swc2(Cpu *cpu, Instruction instruction);

static void Sll(Cpu *cpu, Instruction instruction);
static void Srl(Cpu *cpu, Instruction instruction);
static void Sra(Cpu *cpu, Instruction instruction);
static void Sllv(Cpu *cpu, Instruction instruction);
static void Srlv(Cpu *cpu, Instruction instruction);
static void Srav(Cpu *cpu, Instruction instruction);
static void Jr(Cpu *cpu, Instruction instruction);
static void Jalr(Cpu *cpu, Instruction instruction);
static void Syscall(Cpu *cpu, Instruction instruction);
static void Break(Cpu *cpu, Instruction instruction);
static void Mfhi(Cpu *cpu, Instruction instruction);
static void Mthi(Cpu *cpu, Instruction instruction);
static void Mflo(Cpu *cpu, Instruction instruction);
static void Mtlo(Cpu *cpu, Instruction instruction);
static void Mult(Cpu *cpu, Instruction instruction);
static void Multu(Cpu *cpu, Instruction instruction);
static void Div(Cpu *cpu, Instruction instruction);
static void Divu(Cpu *cpu, Instruction instruction);
static void Add(Cpu *cpu, Instruction instruction);
static void Addu(Cpu *cpu, Instruction instruction);
static void Sub(Cpu *cpu, Instruction instruction);
static void Subu(Cpu *cpu, Instruction instruction);
static void And(Cpu *cpu, Instruction instruction);
static void Or(Cpu *cpu, Instruction instruction);
static void Xor(Cpu *cpu, Instruction instruction);
static void Nor(Cpu *cpu, Instruction instruction);
static void Slt(Cpu *cpu, Instruction instruction);
static void Sltu(Cpu *cpu, Instruction instruction);
static void Bltz(Cpu *cpu, Instruction instruction);
static void Bgez(Cpu *cpu, Instruction instruction);
static void Bltzal(Cpu *cpu, Instruction instruction);
static void Bgezal(Cpu *cpu, Instruction instruction);

static void Rfe(Cpu *cpu, Instruction instruction);

ASSUME_NONNULL_END
