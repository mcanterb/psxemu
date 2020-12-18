#pragma once
#include <stdint.h>

typedef uint32_t InterruptCode;

extern const InterruptCode kInterruptVBlank;
extern const InterruptCode kInterruptGpu;
extern const InterruptCode kInterruptCdrom;
extern const InterruptCode kInterruptDma;
extern const InterruptCode kInterruptTimer0;
extern const InterruptCode kInterruptTimer1;
extern const InterruptCode kInterruptTimer2;
extern const InterruptCode kInterruptControllerMemoryCard;
extern const InterruptCode kInterruptSerialPort;
extern const InterruptCode kInterruptSpu;
extern const InterruptCode kInterruptControllerLightPen;
