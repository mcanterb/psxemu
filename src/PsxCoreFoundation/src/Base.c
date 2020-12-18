#include "Internal.h"
#include "PsxCoreFoundation/String.h"
#include <string.h>

ASSUME_NONNULL_BEGIN

static PCFStringRef PCFBaseToString(PCFObject _Nullable obj);
static bool PCFBaseEquals(PCFObject _Nullable obj, PCFObject _Nullable other);
static void PCFBaseDestructor(PCFObject obj);
static uint32_t PCFBaseHash(PCFObject _Nullable obj);

PCFDestructor kPCFDefaultDestructor = PCFBaseDestructor;
PCFToString kPCFDefaultToString = PCFBaseToString;
PCFEquals kPCFDefaultEquals = PCFBaseEquals;
PCFHash kPCFDefaultHash = PCFBaseHash;

PCFBaseVTable kPCFObjectVTable = {
    .destructor = PCFBaseDestructor,
    .toString = PCFBaseToString,
    .equals = PCFBaseEquals,
    .hash = PCFBaseHash,
};

static PCFStringRef PCFNullString() {
  static PCFStringRef nullString = NULL;
  if (nullString == NULL) {
    nullString = PCFCSTR("NULL");
  }
  return nullString;
}

void PCFBaseConstructor(PCFObject ref, uint32_t typeId) {
  PCFBaseRef base = (PCFBaseRef)ref;
  base->count = 1;
  base->vtable = &kPCFObjectVTable;
  base->type = typeId;
}

void PCFBaseConstructorWithOverrides(PCFObject ref, PCFBaseVTable *vtable,
                                     uint32_t typeId) {
  PCFBaseRef base = (PCFBaseRef)ref;
  base->count = 1;
  base->vtable = vtable;
  base->type = typeId;
}

void PCFBaseMarkObjectImmortal(PCFObject ref) {
  PCFBaseRef base = (PCFBaseRef)ref;
  base->count = UINT32_MAX;
}

PCFObject MOCKABLE(PCFRetain)(PCFObject ref) {
  PCFBaseRef base = (PCFBaseRef)ref;
  if (base->count == UINT32_MAX) {
    return ref;
  }
  base->count++;
  if (base->count == UINT32_MAX) {
    PCFStringRef str = PCFObjectToString(ref);
    PCF_PANIC("Too many references to object! toString = %s", str);
    PCFRelease(str);
    return ref;
  }
  return ref;
}

void MOCKABLE(PCFRelease)(PCFObject _Nullable ref) {
  if (ref == NULL) {
    return;
  }
  PCFBaseRef base = (PCFBaseRef)ref;
  if (base->count == UINT32_MAX) {
    return;
  }
  if (base->count == 0) {
    PCFStringRef str = PCFObjectToString(ref);
    PCF_PANIC("Somehow object has zero references! toString = %s", str);
    PCFRelease(str);
    return;
  }
  base->count--;
  if (base->count == 0) {
    base->vtable->destructor(ref);
  }
}

static PCFStringRef PCFBaseToString(PCFObject _Nullable obj) {
  if (obj == NULL) {
    return PCFNullString();
  }
  PCFBaseRef ref = (PCFBaseRef)obj;
  return PCFStringNewFromFormat(PCFCSTR("PCFObject[type: %d] @ %lld"),
                                ref->type, (long long)obj);
}

static bool PCFBaseEquals(PCFObject _Nullable obj, PCFObject _Nullable other) {
  return obj == other;
}

static void PCFBaseDestructor(PCFObject obj) { free(obj); }

#if UINTPTR_MAX == 0xffffffff
/* 32-bit */
static uint32_t PCFBaseHash(PCFObject _Nullable obj) { return (uint32_t)obj; }
#elif UINTPTR_MAX == 0xffffffffffffffff
/* 64-bit */
static uint32_t PCFBaseHash(PCFObject _Nullable obj) {
  uint64_t val = (uint64_t)obj;
  return (val & 0xFFFFFFFF) | ((val >> 32) & 0xFFFFFFFF);
}
#else
#error Could not detect whether compiling for 32bit or 64bit!
#endif

void *MOCKABLE(PCFMalloc)(size_t capacity) {
  void *result = calloc(capacity, 1);
  if (result == NULL) {
    PCF_PANIC("Could not allocate %d bytes!", capacity);
    return (void *_Nonnull)0; // Should Never Happen
  }
  return (void *_Nonnull)result;
}

void *MOCKABLE(PCFRealloc)(void *original, size_t originalCapacity,
                           size_t capacity) {
  void *result = realloc(original, capacity);
  if (result == NULL) {
    PCF_PANIC("Could not reallocate %d bytes!", capacity);
    return (void *_Nonnull)0; // Should Never Happen
  }
  memset(((uint8_t *)result) + originalCapacity, 0, capacity);
  return (void *_Nonnull)result;
}

void MOCKABLE(PCFPanic)(const char *message) {
  printf("%s\n", message);
  exit(1);
}

PCFStringRef PCFObjectToString(PCFObject _Nullable obj) {
  PCFBaseRef ref = (PCFBaseRef)obj;
  return ref->vtable->toString(obj);
}

bool PCFObjectEquals(PCFObject _Nullable obj1, PCFObject _Nullable obj2) {
  PCFBaseRef ref = (PCFBaseRef)obj1;
  return ref->vtable->equals(obj1, obj2);
}

uint32_t PCFObjectHash(PCFObject _Nullable obj) {
  PCFBaseRef ref = (PCFBaseRef)obj;
  return ref->vtable->hash(obj);
}

MOCK(PCFPanic);
MOCK(PCFRelease);
MOCK(PCFRetain);
MOCK(PCFMalloc);
MOCK(PCFRealloc);

ASSUME_NONNULL_END
