#include "Types.h"

ASSUME_NONNULL_BEGIN

Clock *ClockNew(System *sys);
ClockDeviceHandle ClockAddDevice(Clock *clock, ClockDevice *device);
void ClockDeviceSetDefaultUpdateFrequency(ClockDeviceHandle handle, uint32_t cycles);
void ClockDeviceRequestUpdate(ClockDeviceHandle handle, uint32_t cycles);
uint32_t ClockDeviceCyclesToNextUpdate(ClockDeviceHandle handle);
void ClockResetRealtime(Clock *clock);
void ClockTick(Clock *clock, uint32_t cycles);
void ClockSyncToRealtime(Clock *clock);
double ClockSystemTime(Clock *clock);
double ClockCyclesOfMasterClock(uint32_t cycles);

ASSUME_NONNULL_END
