#pragma once
#include "PsxCoreFoundation/Base.h"

ASSUME_NONNULL_BEGIN

#define PCFCSTR(x) PCFStringIntern(x)
#define PCFDEBUG(format, ...) PCFLOG(PCFLogDebug, format, __VA_ARGS__)
#define PCFWARN(format, ...) PCFLOG(PCFLogWarn, format, __VA_ARGS__)
#define PCFERROR(format, ...) PCFLOG(PCFLogError, format, __VA_ARGS__)
#define PCFLOG_FORMAT(format)                                                  \
  _Generic((format), PCFStringRef : format, const char* : PCFCSTR(format), char*: PCFCSTR(format), char[sizeof( format )]: PCFCSTR(format))
#define PCFLOG(level, format, ...)                                             \
  PCFLog(level, PCFLOG_FORMAT(format), __VA_ARGS__)
#define PCFFORMAT(format, ...)                                                 \
  PCFStringNewFromFormat(PCFLOG_FORMAT(format), __VA_ARGS__)

PCFStringRef PCFStringNewFromCString(const char *cStr);
PCFStringRef PCFStringIntern(const char *str);
PCFStringRef PCFStringNewMutable(size_t initialCapacity);
PCFStringRef PCFStringNewFromFormat(PCFStringRef format, ...);
PCFStringRef PCFStringMakeImmutable(PCFMutableStringRef str);
PCFStringRef PCFStringNewFromError(errno_t error);
char *PCFStringToCString(PCFStringRef str);
PCFMutableStringRef PCFStringConcat(PCFMutableStringRef str,
                                    PCFStringRef toConcat);
PCFStringRef PCFStringConcat(PCFStringRef str1, PCFStringRef str2);
size_t PCFStringLength(PCFStringRef str);
size_t PCFStringStorageSize(PCFStringRef str);
void PCFLog(PCFLogLevel level, PCFStringRef format, ...);
void PCFSetLogLevel(PCFLogLevel level);

ASSUME_NONNULL_END
