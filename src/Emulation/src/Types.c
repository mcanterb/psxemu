#include "Types.h"

ASSUME_NONNULL_BEGIN

const Address kPhysicalAddressMask = 0x1FFFFFFF;
const MemorySegment kMemSegmentMap[8] = {
    UserSegment,    InvalidSegment, InvalidSegment, InvalidSegment,
    KernelSegment0, KernelSegment1, InvalidSegment, KernelSegment2,
};
const MemorySegments kMainSegments =
    UserSegment | KernelSegment0 | KernelSegment1;

PCFStringRef MemorySegmentName(MemorySegment segment) {
  switch (segment) {
  case UserSegment:
    return PCFCSTR("USEG");
  case KernelSegment0:
    return PCFCSTR("KSEG0");
  case KernelSegment1:
    return PCFCSTR("KSEG1");
  case KernelSegment2:
    return PCFCSTR("KSEG2");
  case InvalidSegment:
    return PCFCSTR("InvalidSegment");
  }
}

ASSUME_NONNULL_END
