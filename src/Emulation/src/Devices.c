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
/*
* Add Address Ranges for these later
*
SIMPLE_BUS_DEVICE_DECLARE(Peripherals)
SIMPLE_BUS_DEVICE_DECLARE(MemoryControl2)
SIMPLE_BUS_DEVICE_DECLARE(InterruptControl)
SIMPLE_BUS_DEVICE_DECLARE(Dma)
SIMPLE_BUS_DEVICE_DECLARE(Timers)
SIMPLE_BUS_DEVICE_DECLARE(Cdrom)
SIMPLE_BUS_DEVICE_DECLARE(Mdec)
SIMPLE_BUS_DEVICE_DECLARE(SpuVoice)
SIMPLE_BUS_DEVICE_DECLARE(SpuControl)
SIMPLE_BUS_DEVICE_DECLARE(SpuReverb)
SIMPLE_BUS_DEVICE_DECLARE(SpuMisc)
SIMPLE_BUS_DEVICE_DECLARE(CacheControl)
*/
ASSUME_NONNULL_END
