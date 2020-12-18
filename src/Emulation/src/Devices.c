#include "Devices.h"
#include "Bus.h"
#include "System.h"

ASSUME_NONNULL_BEGIN

SIMPLE_BUS_DEVICE_DEFINITION(Expansion1,
                             NewAddressRange(0x1F000000, 0x1F080000,
                                             kMainSegments),
                             { return 0xFFFFFFFF; })
SIMPLE_BUS_DEVICE_DEFINITION(Expansion2,
                             NewAddressRange(0x1F802000, 0x1F800080,
                                             kMainSegments),
                             { return 0x00000000; })
SIMPLE_BUS_DEVICE_DEFINITION(MemoryControl1,
                             NewAddressRange(0x1F801000, 0x1F801024,
                                             kMainSegments),
                             { return 0x00000000; })
SIMPLE_BUS_DEVICE_DEFINITION(Peripherals,
                             NewAddressRange(0x1F801040, 0x1F801060,
                                             kMainSegments),
                             { return 0x00000000; })
SIMPLE_BUS_DEVICE_DEFINITION(MemoryControl2,
                             NewAddressRange(0x1F801060, 0x1F801064,
                                             kMainSegments),
                             { return 0x00000B88; })
SIMPLE_BUS_DEVICE_DEFINITION(InterruptControl,
                             NewAddressRange(0x1F801070, 0x1F801078,
                                             kMainSegments),
                             { return 0x00000000; })
SIMPLE_BUS_DEVICE_DEFINITION(Dma,
                             NewAddressRange(0x1F801080, 0x1F801100,
                                             kMainSegments),
                             { return 0x00000000; })
SIMPLE_BUS_DEVICE_DEFINITION(Timers,
                             NewAddressRange(0x1F801100, 0x1F801130,
                                             kMainSegments),
                             { return 0x00000000; })
SIMPLE_BUS_DEVICE_DEFINITION(Cdrom,
                             NewAddressRange(0x1F801800, 0x1F801804,
                                             kMainSegments),
                             { return 0x00000000; })
SIMPLE_BUS_DEVICE_DEFINITION(Mdec,
                             NewAddressRange(0x1F801820, 0x1F801828,
                                             kMainSegments),
                             { return 0x00000000; })
SIMPLE_BUS_DEVICE_DEFINITION(SpuVoice,
                             NewAddressRange(0x1F801C00, 0x1F801D80,
                                             kMainSegments),
                             { return 0x00000000; })
SIMPLE_BUS_DEVICE_DEFINITION(SpuControl,
                             NewAddressRange(0x1F801D80, 0x1F801DC0,
                                             kMainSegments),
                             { return 0x00000000; })
SIMPLE_BUS_DEVICE_DEFINITION(SpuReverb,
                             NewAddressRange(0x1F801DC0, 0x1F801E00,
                                             kMainSegments),
                             { return 0x00000000; })
SIMPLE_BUS_DEVICE_DEFINITION(SpuMisc,
                             NewAddressRange(0x1F801E00, 0x1F802000,
                                             kMainSegments),
                             { return 0x00000000; })
SIMPLE_BUS_DEVICE_DEFINITION(CacheControl,
                             NewAddressRange(0xFFFE0130, 0xFFFE0134,
                                             KernelSegment2),
                             { return 0x00000000; })

ASSUME_NONNULL_END
