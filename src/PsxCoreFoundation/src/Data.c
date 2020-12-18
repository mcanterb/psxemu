#include "PsxCoreFoundation/Data.h"
#include "Internal.h"
#include "PsxCoreFoundation/String.h"
#include <stdlib.h>
#include <string.h>

ASSUME_NONNULL_BEGIN

struct __PCFData {
  PCFBase obj;
  size_t capacity;
  uint8_t data[];
};

static PCFStringRef PCFDataToString(PCFObject obj);

PCF_VTABLE(PCFData, { table.toString = PCFDataToString; })

PCFDataRef PCFDataAllocate(size_t capacity) {
  size_t size = sizeof(struct __PCFData) + capacity;
  PCFDataRef data = (PCFDataRef)PCFMalloc(size);
  return data;
}

PCFDataRef PCFDataStackInitialize(void *mem, size_t capacity) {
  PCFDataRef data = (PCFDataRef)mem;
  PCFBaseConstructorWithOverrides(data, PCFDataVTable(), PCFDataTypeId);
  PCFBaseMarkObjectImmortal(data);
  data->capacity = capacity;
  return data;
}

PCFDataRef PCFDataNew(size_t capacity) {
  PCFDataRef result = PCFDataAllocate(capacity);
  PCFBaseConstructorWithOverrides(result, PCFDataVTable(), PCFDataTypeId);
  result->capacity = capacity;
  return result;
}

PCFDataRef PCFDataNewFromPointer(size_t capacity, void *ptr) {
  PCFDataRef result = PCFDataNew(capacity);
  memcpy_s(result->data, capacity, ptr, capacity);
  return result;
}

PCFDataResult PCFDataNewFromFile(PCFStringRef path) {
  FILE *file;
  errno_t error = fopen_s(&file, PCFStringToCString(path), "rb");

  if (error != 0) {
    return PCFDataResultError(PCFStringNewFromError(error));
  }
  fseek(file, 0L, SEEK_END);
  long sz = ftell(file);
  rewind(file);
  PCFDataRef result = PCFDataNew(sz);
  fread_s(result->data, sz, 1, sz, file);
  fclose(file);
  return PCFDataResultSuccess(result);
}

uint8_t PCFDataGetByte(PCFDataRef data, size_t position) {
  if (position > data->capacity - 1) {
    PCF_PANIC("Position >= capacity");
    return 0;
  }
  return data->data[position];
}

void PCFDataSetByte(PCFDataRef data, size_t position, uint8_t byte) {
  if (position > data->capacity - 1) {
    PCF_PANIC("Position >= capacity");
    return;
  }
  data->data[position] = byte;
}

uint16_t PCFDataGetShort(PCFDataRef data, size_t position) {
  if (position > data->capacity - 2) {
    PCF_PANIC("Position >= capacity");
    return 0;
  }
  return ((uint16_t *)data->data)[position >> 1];
}

void PCFDataSetShort(PCFDataRef data, size_t position, uint16_t value) {
  if (position > data->capacity - 2) {
    PCF_PANIC("Position >= capacity");
    return;
  }
  ((uint16_t *)data->data)[position >> 1] = value;
}

uint32_t PCFDataGetInt(PCFDataRef data, size_t position) {
  if (position > data->capacity - 4) {
    PCF_PANIC("Position >= capacity");
    return 0;
  }
  return ((uint32_t *)data->data)[position >> 2];
}

void PCFDataSetInt(PCFDataRef data, size_t position, uint32_t value) {
  if (position > data->capacity - 4) {
    PCF_PANIC("Position >= capacity");
    return;
  }
  ((uint32_t *)data->data)[position >> 2] = value;
}

PCFStringRef PCFDataToString(PCFObject obj) {
  return PCFStringNewFromFormat(PCFCSTR("PCFData {capacity = %d}"),
                                ((PCFDataRef)obj)->capacity);
}

size_t PCFDataSizeOf(size_t capacity) {
  return sizeof(struct __PCFData) + capacity;
}

size_t PCFDataCopyInto(PCFDataRef data, void *ptr, size_t capacity) {
  size_t toCopy = capacity < data->capacity ? capacity : data->capacity;
  errno_t error = memcpy_s(ptr, toCopy, data->data, toCopy);
  if (error != 0) {
    PCFStringRef errorString = PCFStringNewFromError(error);
    PCF_PANIC("Failed to copy data: %s", errorString);
    PCFRelease(errorString);
    return 0;
  }
  return toCopy;
}

size_t PCFDataCapacity(PCFDataRef data) { return data->capacity; }

ASSUME_NONNULL_END
