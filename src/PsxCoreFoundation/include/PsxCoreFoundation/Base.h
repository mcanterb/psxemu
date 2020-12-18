#pragma once
#include "Macros.h"
#include "Mock.h"
#include "Types.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__clang__)
#define ASSUME_NONNULL_BEGIN _Pragma("clang assume_nonnull begin")
#define ASSUME_NONNULL_END _Pragma("clang assume_nonnull end")
#else
#define ASSUME_NONNULL_BEGIN
#define ASSUME_NONNULL_END
#define _Nonnull
#define _Nullable
#define _Null_unspecified
#define __nullable
#define __nonnull
#endif

typedef void *PCFObject;

ASSUME_NONNULL_BEGIN
typedef void (*PCFDestructor)(PCFObject);
typedef PCFStringRef _Nonnull (*PCFToString)(PCFObject _Nullable);
typedef bool (*PCFEquals)(PCFObject _Nullable, PCFObject _Nullable);
typedef uint32_t (*PCFHash)(PCFObject _Nullable);

typedef struct __PCFBaseVTable {
  PCFDestructor destructor;
  PCFToString toString;
  PCFEquals equals;
  PCFHash hash;
} PCFBaseVTable;

extern PCFBaseVTable kPCFObjectVTable;
extern PCFDestructor kPCFDefaultDestructor;
extern PCFToString kPCFDefaultToString;
extern PCFEquals kPCFDefaultEquals;
extern PCFHash kPCFDefaultHash;

typedef struct __PCFBase {
  uint8_t type;
  uint32_t count;
  PCFBaseVTable *vtable;
} PCFBase;
typedef PCFBase *PCFBaseRef;

void PCFBaseConstructor(PCFObject ref, uint32_t typeId);
void PCFBaseConstructorWithOverrides(PCFObject ref, PCFBaseVTable *vtable,
                                     uint32_t typeId);
void PCFBaseMarkObjectImmortal(PCFObject ref);

PCFObject PCFRetain(PCFObject ref);
void PCFRelease(PCFObject _Nullable ref);
void PCFPanic(const char *message);
void *PCFMalloc(size_t capacity);
void *PCFRealloc(void *original, size_t originalCapacity, size_t capacity);
static inline void *PCFStackAlloc(size_t capacity) {
  void *result = _alloca(capacity);
  memset(result, 0, capacity);
  return result;
}

PCFStringRef PCFObjectToString(PCFObject _Nullable obj);
bool PCFObjectEquals(PCFObject _Nullable obj1, PCFObject _Nullable obj2);
uint32_t PCFObjectHash(PCFObject _Nullable);

char *PCFStringToCString(PCFStringRef str);

RESULT_TYPE(PCFData, PCFDataRef)

typedef struct __PCFResult {
  bool successful;
  PCFStringRef _Nullable error;
} PCFResult;

inline void PCFResultOrPanic(PCFResult result) {
  if (result.successful) {
    return;
  }
  PCF_PANIC("Failed to get result: %s", PCFStringToCString(result.error));
  PCFRelease(result.error);
}

inline PCFResult PCFResultSuccess() {
  PCFResult result = {.successful = true};
  return result;
}

inline PCFResult PCFResultError(PCFStringRef error) {
  PCFResult result = {.successful = false, .error = error};
  return result;
}

inline void PCFResultReleaseError(PCFResult result) {
  PCFRelease(result.error);
}

DECLARE_MOCK(PCFPanic);
DECLARE_MOCK(PCFRelease);
DECLARE_MOCK(PCFRetain);
DECLARE_MOCK(PCFMalloc);
DECLARE_MOCK(PCFRealloc);

ASSUME_NONNULL_END
