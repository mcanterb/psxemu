#include "Exceptions.h"

const ExceptionCode kExceptionExternalInterrupt = 0;
const ExceptionCode kExceptionTlbModification = 1;
const ExceptionCode kExceptionTlbMissFetch = 2;
const ExceptionCode kExceptionTlbMissStore = 3;
const ExceptionCode kExceptionAddressErrorFetch = 4;
const ExceptionCode kExceptionAddressErrorStore = 5;
const ExceptionCode kExceptionBusErrorFetch = 6;
const ExceptionCode kExceptionBusErrorStore = 7;
const ExceptionCode kExceptionSyscall = 8;
const ExceptionCode kExceptionBreakPoint = 9;
const ExceptionCode kExceptionReservedInstruction = 10;
const ExceptionCode kExceptionCoProcessorUnusable = 11;
const ExceptionCode kExceptionOverflow = 12;
