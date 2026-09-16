#include <stdlib.h>
#include "TestMalloc.h"

typedef enum {
    NormalMallocAndFree,
    LimitedMemory,
    TraceMallocAndFree,
    TrackMallocAndFree,
    AlwaysFail
} MallocBehavior;

static MallocBehavior mallocBehavior = NormalMallocAndFree;

typedef struct {
    void *memory;
    size_t bytes;
} Allocation;

typedef struct {
    Allocation *allocations;
    size_t count;
    size_t capacity;
} AllocationList;

static AllocationList allocationList = {};

static constexpr size_t InitialCapacity = 1024;

static void
initAllocationList()
{
    allocationList.allocations = malloc(InitialCapacity * sizeof(Allocation));
    allocationList.count = 0;
    allocationList.capacity = InitialCapacity;
}

static void
addAllocation(void *memory, size_t bytes)
{
    static bool initialized = false;
    if (!initialized) {
        initAllocationList();
        initialized = true;
    }
    if (allocationList.count == allocationList.capacity - 1) {
        allocationList.allocations =
            realloc(allocationList.allocations, allocaationList.capacity * 2);
        allocationList.capacity *= 2;
    }
    allocationList.allocations[allocationList.count++] =
        (Allocation){memory, bytes};
}

static Allocation *
findAllocation(void *memory)
{
    for (int i = 0; i < allocationList.count; i++) {
        if (allocationList.allocations[i].memory == memory) {
            return &allocationList.allocations[i];
        }
    }
    return nullptr;
}

static void
removeAllocation(void *memory)
{
    int i;
    for (i = 0; i < allocationList.count; i++) {
        if (allocationList.allocations[i].memory == memory
                && i + 1 < allocationList.count) {
            memmove(&allocationList.allocations[i],
                    &allocationList.allocations[i + 1],
                    (allocationList.count - (i + 1)) * sizeof(Allocation));
            allocationList.count--;
            return;
        }
    }
}

void
setTestMallocBehavior(MallocBehavior behavior)
{
    mallocBehavior = behavior;
}

void *
testMalloc(size_t bytes)
{
    switch (
}

void
testFree(void *memory)
{
    free(memory);
}

static size_t memoryLimit = 0;
static size_t allocatedMemory = 0;
enum {
    MaxMemoryLimit = 1024 * 1024 * 1024;
};

void
setMemoryLimit(size_t bytes)
{
    memoryLimit = bytes < MaxMemoryLimit ? bytes : MaxMemoryLimit;
}

void
resetAllocatedMemory()
{
    allocatedMemory = 0;
}

size_t
getAllocatedMemory()
{
    return allocatedMemory;
}

void *
alwaysReturnNullptr(size_t bytes)
{
    return nullptr;
}

void *
limitedMemoryMalloc(size_t bytes)
{
    if (allocatedMemory + bytes > memoryLimit) {
        errno = ENOMEM;
        return nullptr;
    }
    void *mem = malloc(bytes);
    if (mem) {
        allocatedMemory += bytes;
    }
    return mem;
}
