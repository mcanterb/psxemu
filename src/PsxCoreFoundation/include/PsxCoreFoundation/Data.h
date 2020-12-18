#pragma once
#include "PsxCoreFoundation/Base.h"

ASSUME_NONNULL_BEGIN

PCFDataRef PCFDataStackInitialize(void *mem, size_t capacity);

PCFDataRef PCFDataNew(size_t capacity);

PCFDataRef PCFDataNewFromPointer(size_t capacity, void *ptr);

PCFDataResult PCFDataNewFromFile(PCFStringRef path);

uint8_t PCFDataGetByte(PCFDataRef data, size_t position);

void PCFDataSetByte(PCFDataRef data, size_t position, uint8_t value);

uint16_t PCFDataGetShort(PCFDataRef data, size_t position);

void PCFDataSetShort(PCFDataRef data, size_t position, uint16_t value);

uint32_t PCFDataGetInt(PCFDataRef data, size_t position);

void PCFDataSetInt(PCFDataRef data, size_t position, uint32_t value);

size_t PCFDataSizeOf(size_t capacity);

size_t PCFDataCopyInto(PCFDataRef data, void *ptr, size_t capacity);

size_t PCFDataCapacity(PCFDataRef data);

ASSUME_NONNULL_END
