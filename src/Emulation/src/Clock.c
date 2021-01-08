#include "Clock.h"
#include "System.h"
#include <SDL.h>
#include <math.h>

ASSUME_NONNULL_BEGIN

// Time in Nanos
#define kMaxNumOfDevices 10
#define kMasterClockTickFrequency (1000.0 / 33.868800)
#define kDefaultUpdateFrequency (1000000000.0 / 60.0)

typedef struct __ClockDeviceEntry {
  ClockDevice device;
  double nextUpdate;
  double updateFrequency;
  double lastUpdateTime;
} ClockDeviceEntry;

struct __Clock {
  double realTimePerTick;
  uint64_t realLastSync;
  double systemTime;
  double lastSync;
  size_t numOfDevices;
  ClockDeviceEntry devices[kMaxNumOfDevices];
  ClockDeviceEntry *_Nullable heap[kMaxNumOfDevices];
};

static inline ClockDeviceEntry *ClockDeviceHandleGetEntry(ClockDeviceHandle handle) {
  return &handle.clock->devices[handle.index];
}

static void HeapSwap(ClockDeviceEntry **a, ClockDeviceEntry **b) {
  ClockDeviceEntry *t;
  t = *a;
  *a = *b;
  *b = t;
}

// function to get right child of a node of a tree
static int HeapGetRightChild(Clock *clock, size_t index) {
  index++;
  if ((((2 * index) + 1) <= clock->numOfDevices))
    return (2 * index);
  return -1;
}

// function to get left child of a node of a tree
static int HeapGetLeftChild(Clock *clock, size_t index) {
  index++;
  if (((2 * index) <= clock->numOfDevices))
    return 2 * index - 1;
  return -1;
}

// function to get the parent of a node of a tree
static int HeapGetParent(Clock *clock, size_t index) {
  index++;
  if ((index <= clock->numOfDevices)) {
    return (index / 2) - 1;
  }
  return -1;
}

static void HeapMinHeapify(Clock *clock, size_t index) {
  ClockDeviceEntry **heap = clock->heap;
  int left_child_index = HeapGetLeftChild(clock, index);
  int right_child_index = HeapGetRightChild(clock, index);

  // finding smallest among index, left child and right child
  int smallest = index;

  if ((left_child_index <= clock->numOfDevices) && (left_child_index > 0)) {
    if (heap[left_child_index]->nextUpdate < heap[smallest]->nextUpdate) {
      smallest = left_child_index;
    }
  }

  if ((right_child_index <= clock->numOfDevices && (right_child_index > 0))) {
    if (heap[right_child_index]->nextUpdate < heap[smallest]->nextUpdate) {
      smallest = right_child_index;
    }
  }

  // smallest is not the node, node is not a heap
  if (smallest != index) {
    HeapSwap(&heap[index], &heap[smallest]);
    HeapMinHeapify(clock, smallest);
  }
}

static ClockDeviceEntry *HeapMinimum(Clock *clock) { return clock->heap[0]; }

static void HeapDecreaseNextUpdate(Clock *clock, size_t index) {
  ClockDeviceEntry **heap = clock->heap;
  while ((index > 0) && (heap[HeapGetParent(clock, index)]->nextUpdate > heap[index]->nextUpdate)) {
    HeapSwap(&heap[index], &heap[HeapGetParent(clock, index)]);
    index = HeapGetParent(clock, index);
  }
}

static void HeapIncreaseNextUpdate(Clock *clock, size_t index) { HeapMinHeapify(clock, index); }

static inline ClockDeviceEntry NewClockDeviceEntry(ClockDevice *device) {
  ClockDeviceEntry entry = {
      .device = *device, .nextUpdate = kDefaultUpdateFrequency, .updateFrequency = kDefaultUpdateFrequency};
  return entry;
}

static inline ClockDeviceHandle NewClockDeviceHandle(Clock *clock, size_t index) {
  ClockDeviceHandle handle = {.clock = clock, .index = index};
  return handle;
}

static size_t FindHeapIndexForHandle(ClockDeviceHandle handle) {
  Clock *clock = handle.clock;
  ClockDeviceEntry **heap = clock->heap;
  ClockDeviceEntry *target = ClockDeviceHandleGetEntry(handle);
  size_t i;
  for (i = 0; i < clock->numOfDevices; i++) {
    if (heap[i] == target) {
      return i;
    }
  }
  PCF_PANIC("Received an invalid ClockDeviceHandle! Index in Handle: %d. Devices attached to clock: %d", handle.index,
            clock->numOfDevices);
  return -1;
}

Clock *ClockNew(System *sys) {
  Clock *clock = (Clock *)SystemArenaAllocate(sys, sizeof(*clock));
  clock->numOfDevices = 0;
  clock->systemTime = 0.0;
  clock->lastSync = 0.0;
  return clock;
}

void ClockResetRealtime(Clock *clock) {
  clock->realTimePerTick = 1000000000.0 / (double)SDL_GetPerformanceFrequency();
  clock->realLastSync = SDL_GetPerformanceCounter();
}

ClockDeviceHandle ClockAddDevice(Clock *clock, ClockDevice *device) {
  if (clock->numOfDevices == kMaxNumOfDevices) {
    PCF_PANIC("Too many devices registered with clock!");
  }
  size_t index = clock->numOfDevices;
  clock->numOfDevices++;
  clock->devices[index] = NewClockDeviceEntry(device);
  clock->heap[index] = &clock->devices[index];
  return NewClockDeviceHandle(clock, index);
}

void ClockDeviceSetDefaultUpdateFrequency(ClockDeviceHandle handle, uint32_t cycles) {
  ClockDeviceEntry *entry = ClockDeviceHandleGetEntry(handle);
  double newUpdateFrequency = entry->device.nanoSecsPerCycle * cycles;
  double systemTime = handle.clock->systemTime;
  entry->updateFrequency = newUpdateFrequency;
  if (entry->nextUpdate > (systemTime + newUpdateFrequency)) {
    entry->nextUpdate = (systemTime + newUpdateFrequency);
    HeapDecreaseNextUpdate(handle.clock, FindHeapIndexForHandle(handle));
  }
}

void ClockDeviceRequestUpdate(ClockDeviceHandle handle, uint32_t cycles) {
  ClockDeviceEntry *entry = ClockDeviceHandleGetEntry(handle);
  double nextUpdate = entry->device.nanoSecsPerCycle * cycles;
  if (entry->nextUpdate > nextUpdate) {
    entry->nextUpdate = nextUpdate;
    HeapDecreaseNextUpdate(handle.clock, FindHeapIndexForHandle(handle));
  }
}

static void UpdateDeviceEntry(ClockDeviceEntry *entry, double systemTime) {
  uint32_t cycles = (systemTime - entry->lastUpdateTime) / entry->device.nanoSecsPerCycle;
  entry->lastUpdateTime = systemTime;
  entry->device.update(entry->device.context, cycles);
}

void ClockTick(Clock *clock, uint32_t cycles) {
  double timeTick = cycles * kMasterClockTickFrequency;
  double systemTime = clock->systemTime + timeTick;
  clock->systemTime = systemTime;
  if (clock->numOfDevices > 0) {
    ClockDeviceEntry *entry = HeapMinimum(clock);
    while (entry->nextUpdate < systemTime) {
      UpdateDeviceEntry(entry, systemTime);
      entry->nextUpdate = systemTime + entry->updateFrequency;
      HeapIncreaseNextUpdate(clock, 0);
      entry = HeapMinimum(clock);
    }
  }
}

uint32_t ClockDeviceCyclesToNextUpdate(ClockDeviceHandle handle) {
  ClockDeviceEntry *entry = ClockDeviceHandleGetEntry(handle);
  return (uint32_t)ceil(entry->nextUpdate / entry->device.nanoSecsPerCycle);
}

void ClockSyncToRealtime(Clock *clock) {
  double systemTime = clock->systemTime - clock->lastSync;
  double realTime = (SDL_GetPerformanceCounter() - clock->realLastSync) * clock->realTimePerTick;
  int32_t delayAmount = (systemTime - realTime) / 1000000.0;
  if (delayAmount > 0) {
    SDL_Delay(delayAmount);
  }
  // Uncomment this to provide a rough indicator of the
  // performance of the emulator. As long as delayAmount
  // is positive, our emulator is fast enough to emulate the
  // Playstation in realtime.
  // printf("Delay = %d\n", delayAmount);
  clock->lastSync = clock->systemTime;
  clock->realLastSync = SDL_GetPerformanceCounter();
}

double ClockSystemTime(Clock *clock) { return clock->systemTime; }

double ClockCyclesOfMasterClock(uint32_t cycles) { return cycles * kMasterClockTickFrequency; }

ASSUME_NONNULL_END
