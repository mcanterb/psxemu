#pragma once
#include "Exceptions.h"
#include "Interrupts.h"
#include <PsxCoreFoundation/Base.h>
#include <PsxCoreFoundation/String.h>
#include <malloc.h>
#include <stdbool.h>

ASSUME_NONNULL_BEGIN

typedef uint32_t Address;

extern const Address kPhysicalAddressMask;

#define ADDR_FORMAT "0x%08x"

#define PHYSICAL(addr) (addr & kPhysicalAddressMask)
#define SEGMENT(addr) MemorySegmentForAddress(addr)
#define ADDRESS_UNPACK(addr)                                                   \
  Address addr##Physical = PHYSICAL(addr);                                     \
  MemorySegment addr##Segment = SEGMENT(addr)

#define BUS_DEVICE_FUNCS(type)                                                 \
  uint32_t type##Read32(type *mem, MemorySegment segment, Address address);    \
  uint16_t type##Read16(type *mem, MemorySegment segment, Address address);    \
  uint8_t type##Read8(type *mem, MemorySegment segment, Address address);      \
  void type##Write32(type *mem, MemorySegment segment, Address address,        \
                     uint32_t data);                                           \
  void type##Write16(type *mem, MemorySegment segment, Address address,        \
                     uint16_t data);                                           \
  void type##Write8(type *mem, MemorySegment segment, Address address,         \
                    uint8_t data);
#define SIMPLE_BUS_DEVICE_DECLARE(type)                                        \
  void type##New(System *sys, Bus *bus);                                       \
  uint32_t type##Read32(void *_Nullable device, MemorySegment segment,         \
                        Address address);                                      \
  uint16_t type##Read16(void *_Nullable device, MemorySegment segment,         \
                        Address address);                                      \
  uint8_t type##Read8(void *_Nullable device, MemorySegment segment,           \
                      Address address);                                        \
  void type##Write32(void *_Nullable device, MemorySegment segment,            \
                     Address address, uint32_t data);                          \
  void type##Write16(void *_Nullable device, MemorySegment segment,            \
                     Address address, uint16_t data);                          \
  void type##Write8(void *_Nullable device, MemorySegment segment,             \
                    Address address, uint8_t data);

#define SIMPLE_BUS_DEVICE_DEFINITION(type, addrRange, ...)                     \
  void type##New(System *sys, Bus *bus) {                                      \
    BusDevice device = {.context = NULL,                                       \
                        .read32 = type##Read32,                                \
                        .read16 = type##Read16,                                \
                        .read8 = type##Read8,                                  \
                        .write32 = type##Write32,                              \
                        .write16 = type##Write16,                              \
                        .write8 = type##Write8};                               \
    PCFResultOrPanic(BusRegisterDevice(bus, &device, addrRange));              \
  }                                                                            \
  uint32_t type##Read32(void *_Nullable device, MemorySegment segment,         \
                        Address address) {                                     \
    PCFWARN("Unhandled Read to " #type " at offset 0x%02x", address);          \
    EXPAND(__VA_ARGS__)                                                        \
  }                                                                            \
  uint16_t type##Read16(void *_Nullable device, MemorySegment segment,         \
                        Address address) {                                     \
    return (uint16_t)type##Read32(device, segment, address);                   \
  }                                                                            \
  uint8_t type##Read8(void *_Nullable device, MemorySegment segment,           \
                      Address address) {                                       \
    return (uint8_t)type##Read32(device, segment, address);                    \
  }                                                                            \
  void type##Write32(void *_Nullable device, MemorySegment segment,            \
                     Address address, uint32_t value) {                        \
    PCFWARN("Unhandled Write to " #type                                        \
            " at offset 0x%02x with value " ADDR_FORMAT,                       \
            address, value);                                                   \
  }                                                                            \
  void type##Write16(void *_Nullable device, MemorySegment segment,            \
                     Address address, uint16_t value) {                        \
    type##Write32(device, segment, address, (uint32_t)value);                  \
  }                                                                            \
  void type##Write8(void *_Nullable device, MemorySegment segment,             \
                    Address address, uint8_t value) {                          \
    type##Write32(device, segment, address, (uint32_t)value);                  \
  }

typedef enum {
  InvalidSegment = 0,
  UserSegment = 1,
  KernelSegment0 = 2,
  KernelSegment1 = 4,
  KernelSegment2 = 8
} MemorySegment;

extern const MemorySegment kMemSegmentMap[8];

typedef uint8_t MemorySegments;

extern const MemorySegments kMainSegments;

typedef uint8_t Register;

typedef uint32_t (*Read32)(void *_Nullable, MemorySegment, Address);
typedef uint16_t (*Read16)(void *_Nullable, MemorySegment, Address);
typedef uint8_t (*Read8)(void *_Nullable, MemorySegment, Address);
typedef void (*Write32)(void *_Nullable, MemorySegment, Address, uint32_t);
typedef void (*Write16)(void *_Nullable, MemorySegment, Address, uint16_t);
typedef void (*Write8)(void *_Nullable, MemorySegment, Address, uint8_t);

typedef enum {
  Coprocessor0 = 0,
  Coprocessor1,
  Coprocessor2,
  Coprocessor3
} Coprocessor;

typedef struct __SystemException {
  ExceptionCode code;
  Address address;
} SystemException;

typedef struct __BusDevice {
  void *_Nullable context;
  Read32 read32;
  Read16 read16;
  Read8 read8;
  Write32 write32;
  Write16 write16;
  Write8 write8;
} BusDevice;

struct __Bus;
typedef struct __Bus Bus;

typedef struct __AddressRange {
  Address start;
  Address end;
  MemorySegments segments;
} AddressRange;

struct __Cpu;
typedef struct __Cpu Cpu;

struct __Bios;
typedef struct __Bios Bios;

struct __System;
typedef struct __System System;

struct __Memory;
typedef struct __Memory Memory;

struct __Gpu;
typedef struct __Gpu Gpu;

typedef uint32_t GpuPacket;

typedef union __GpuCommand {
  uint32_t value;
  struct __attribute__((packed)) __GpuCommandDecomposed {
    uint32_t parameters : 24;
    uint32_t command : 8;
  } parsed;
} GpuCommand;

static inline GpuCommand GpuPacketToCommand(GpuPacket packet) {
  GpuCommand command = {.value = packet};
  return command;
}

static inline SystemException NewSystemException(ExceptionCode code,
                                                 Address address) {
  SystemException exception = {.code = code, .address = address};
  return exception;
}

static inline MemorySegment MemorySegmentForAddress(Address address) {
  return kMemSegmentMap[address >> 29];
}

static inline bool MemorySegmentsContain(MemorySegments segments,
                                         MemorySegment segment) {
  return (segments & segment) != 0;
}

static inline AddressRange NewAddressRange(Address start, Address end,
                                           MemorySegments segments) {
  AddressRange r = {
      .start = PHYSICAL(start), .end = PHYSICAL(end), .segments = segments};
  return r;
}

static inline bool InRange(const AddressRange *range, Address addr) {
  ADDRESS_UNPACK(addr);
  if (MemorySegmentsContain(range->segments, addrSegment)) {
    return addrPhysical >= range->start && addrPhysical < range->end;
  }
  return false;
}

static inline int32_t RangeCompare(AddressRange r1, AddressRange r2) {
  if (r1.start < r2.start)
    return -1;
  if (r1.start == r2.start)
    return 0;
  return 1;
}

PCFStringRef MemorySegmentName(MemorySegment segment);
ASSUME_NONNULL_END
