#include "PsxCoreFoundation\String.h"
#include "Internal.h"
#include "utf8.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ASSUME_NONNULL_BEGIN

#define ASSERT_MUTABLE(str, ret)                                               \
  do {                                                                         \
    if (str->isMutable == 0) {                                                 \
      PCF_PANIC("Cannot perform operation on immutable string");               \
      return ret;                                                              \
    }                                                                          \
  } while (0)

struct __PCFString {
  PCFBase obj;
  uint32_t isInlined : 1;
  uint32_t isMutable : 1;
  uint32_t isConstant : 1;
  size_t capacity;
  size_t stringEnd;
  union {
    char inlined[];
    char *ptr;
  } strData;
};

typedef struct __PCFInternStringEntry {
  PCFStringRef value;
  struct __PCFInternStringEntry *next;
} PCFInternStringEntry;

const size_t PCFInternedBuckets = 1009;
static PCFInternStringEntry PCFInternedStrings[PCFInternedBuckets] = {};

static PCFLogLevel PCFCurrentLogLevel = PCFLogDebug;

static PCFStringRef PCFStringToString(PCFObject ref) {
  return (PCFStringRef)ref;
}
static void PCFStringDestructor(PCFObject str);

PCF_VTABLE(PCFString, {
  table.destructor = PCFStringDestructor;
  table.toString = PCFStringToString;
})

static void PCFStringDestructor(PCFObject obj) {
  PCFStringRef str = (PCFStringRef)obj;
  if (!str->isInlined && !str->isConstant) {
    free(str->strData.ptr);
  }
  kPCFDefaultDestructor(str);
}

static PCFStringRef PCFStringNewInlinedWithCapacity(size_t capacity) {
  PCFStringRef result = (PCFStringRef)PCFMalloc(sizeof(struct __PCFString) -
                                                sizeof(char *) + capacity);
  PCFBaseConstructorWithOverrides(&result->obj, PCFStringVTable(),
                                  PCFStringTypeId);
  result->isInlined = 1;
  result->isConstant = 0;
  result->isMutable = 0;
  result->capacity = capacity;
  result->stringEnd = 0;
  return result;
}

PCFStringRef PCFStringNewFromCStringConstant(const char *cStr) {
  PCFStringRef result = (PCFStringRef)PCFMalloc(sizeof(struct __PCFString));
  PCFBaseConstructorWithOverrides(&result->obj, PCFStringVTable(),
                                  PCFStringTypeId);
  result->isInlined = 0;
  result->isMutable = 0;
  result->isConstant = 1;
  result->capacity = 0;
  result->stringEnd = strlen(cStr);
  result->strData.ptr = (char *)cStr;
  PCFBaseMarkObjectImmortal(result);
  return result;
}

PCFStringRef PCFStringNewFromCString(const char *cStr) {
  size_t length = strlen(cStr);
  PCFStringRef result =
      (PCFStringRef)PCFMalloc(sizeof(struct __PCFString) + length + 1);
  PCFBaseConstructorWithOverrides(&result->obj, PCFStringVTable(),
                                  PCFStringTypeId);
  result->isInlined = 1;
  result->isMutable = 0;
  result->isConstant = 0;
  result->capacity = 0;
  result->stringEnd = length;
  memcpy(result->strData.inlined, cStr, length + 1);
  return result;
}

PCFStringRef PCFStringNewMutable(size_t initialCapacity) {
  PCFStringRef result =
      (PCFStringRef)PCFMalloc(sizeof(struct __PCFString) + initialCapacity);
  PCFBaseConstructorWithOverrides(&result->obj, PCFStringVTable(),
                                  PCFStringTypeId);
  result->isInlined = 0;
  result->isMutable = 0;
  result->isConstant = 1;
  result->capacity = initialCapacity;
  result->stringEnd = 0;
  result->strData.ptr = (char *)PCFMalloc(initialCapacity);
  return result;
}

PCFStringRef PCFStringNewFromFormatVarargs(PCFStringRef format, va_list argp) {
  va_list argp2;
  va_copy(argp2, argp);
  uint32_t size = PCF_vsnprintf(NULL, 0, PCFStringToCString(format), argp) + 1;
  PCFStringRef result = PCFStringNewInlinedWithCapacity(size);
  PCF_vsnprintf((char *)result->strData.inlined, size,
                PCFStringToCString(format), argp2);
  va_end(argp2);
  return result;
}

PCFStringRef PCFStringNewFromFormat(PCFStringRef format, ...) {
  va_list argp;
  va_start(argp, format);
  PCFStringRef result = PCFStringNewFromFormatVarargs(format, argp);
  va_end(argp);
  return result;
}

PCFMutableStringRef PCFStringConcatMutable(PCFMutableStringRef str,
                                           PCFStringRef toConcat) {
  ASSERT_MUTABLE(str, (PCFMutableStringRef)1);
  size_t remaining = str->capacity - str->stringEnd;
  if (remaining > toConcat->stringEnd) {
    memcpy(str->strData.ptr, PCFStringToCString(toConcat),
           toConcat->stringEnd + 1);
  } else {
    size_t newCapacity = toConcat->stringEnd - remaining;
    str->strData.ptr =
        (char *)PCFRealloc(str->strData.ptr, str->capacity, newCapacity);
    memcpy(str->strData.ptr + str->stringEnd, PCFStringToCString(toConcat),
           toConcat->stringEnd + 1);
  }
  return str;
}

PCFStringRef PCFStringConcat(PCFStringRef str1, PCFStringRef str2) {
  size_t size = str1->stringEnd + str2->stringEnd + 1;
  PCFStringRef result = PCFStringNewInlinedWithCapacity(size);
  memcpy(result->strData.inlined, PCFStringToCString(str1), str1->stringEnd);
  memcpy(result->strData.inlined + str1->stringEnd, PCFStringToCString(str2),
         str2->stringEnd);
  return result;
}

PCFStringRef PCFStringNewFromError(errno_t error) {
  char buffer[256];
  strerror_s(buffer, 255, error);
  return PCFStringNewFromCString(buffer);
}

size_t PCFStringLength(PCFStringRef str) {
  return utf8len(PCFStringToCString(str));
}

char *PCFStringToCString(PCFStringRef str) {
  if (str->isInlined) {
    return str->strData.inlined;
  }
  return str->strData.ptr;
}

size_t PCFStringStorageSize(PCFStringRef str) { return str->stringEnd + 1; }

static PCFStringRef PCFStringInternWithEntry(PCFInternStringEntry *entry,
                                             const char *str) {
  if (entry->value != NULL && entry->value->strData.ptr == str) {
    return entry->value;
  }
  if (entry->next == NULL) {
    PCFInternStringEntry *nextEntry =
        (PCFInternStringEntry *)PCFMalloc(sizeof(PCFInternStringEntry));
    nextEntry->value = PCFStringNewFromCStringConstant(str);
    entry->next = nextEntry;
    return nextEntry->value;
  }
  return PCFStringInternWithEntry(entry->next, str);
}

PCFStringRef PCFStringIntern(const char *str) {
  uint32_t hash = kPCFDefaultHash((void *)str);
  size_t bucket = hash % PCFInternedBuckets;
  PCFInternStringEntry *entry = &PCFInternedStrings[bucket];
  if (entry->value == NULL) {
    entry->value = PCFStringNewFromCStringConstant(str);
    return entry->value;
  }
  if (entry->value->strData.ptr == str) {
    return entry->value;
  }
  return PCFStringInternWithEntry(entry, str);
}

PCFStringRef PCFLogLevelName(PCFLogLevel name) {
  switch (name) {
  case PCFLogDebug:
    return PCFCSTR("DEBUG");
  case PCFLogWarn:
    return PCFCSTR("WARN");
  case PCFLogError:
    return PCFCSTR("ERROR");
  default:
    return PCFCSTR("???");
  }
}

void PCFLog(PCFLogLevel level, PCFStringRef format, ...) {
  if (PCFCurrentLogLevel > level) {
    return;
  }
  printf("[%s] ", PCFStringToCString(PCFLogLevelName(level)));
  va_list argp;
  va_start(argp, format);
  PCFStringRef formatted = PCFStringNewFromFormatVarargs(format, argp);
  va_end(argp);
  printf("%s\n", PCFStringToCString(formatted));
  PCFRelease(formatted);
}

void PCFSetLogLevel(PCFLogLevel level) { PCFCurrentLogLevel = level; }

ASSUME_NONNULL_END
