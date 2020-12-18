#pragma once
#include <stdint.h>

typedef uint8_t ExceptionCode;

extern const ExceptionCode kExceptionExternalInterrupt;
extern const ExceptionCode kExceptionTlbModification;
extern const ExceptionCode kExceptionTlbMissFetch;
extern const ExceptionCode kExceptionTlbMissStore;
extern const ExceptionCode kExceptionAddressErrorFetch;
extern const ExceptionCode kExceptionAddressErrorStore;
extern const ExceptionCode kExceptionBusErrorFetch;
extern const ExceptionCode kExceptionBusErrorStore;
extern const ExceptionCode kExceptionSyscall;
extern const ExceptionCode kExceptionBreakPoint;
extern const ExceptionCode kExceptionReservedInstruction;
extern const ExceptionCode kExceptionCoProcessorUnusable;
extern const ExceptionCode kExceptionOverflow;
