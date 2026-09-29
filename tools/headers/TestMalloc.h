#ifndef TEST_MALLOC_H
#define TEST_MALLOC_H
typedef enum {
    NormalBehavior = 0,
    AlwaysFail = 1,
    LimitMemory = 2,
    LogMemoryUsage = 4,
    TrackMemoryUsage = 8
} MallocBehavior;

void
setMallocBehavior(MallocBehavior behavior);

void *
testMalloc(size_t bytes);

void *
testRealloc(void *memory, size_t bytes);

void
testFree(void *memory);

void
setMemoryLimit(size_t bytes);

void
resetAllocatedMemory();

size_t
getAllocatedMemory();

unsigned
getDoubleFreeAttempts();

void
resetDoubleFreeAttempts();
#endif
