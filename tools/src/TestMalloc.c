#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "TestMalloc.h"

static MallocBehavior mallocBehavior = NormalBehavior;

static inline bool alwaysFail()
{
    return mallocBehavior == AlwaysFail;
}

static inline bool limitMemory()
{
    return (mallocBehavior & LimitMemory) != 0;
}

static inline bool logMemoryUsage()
{
    return (mallocBehavior & LogMemoryUsage) != 0;
}

static inline bool trackMemoryUsage()
{
    return (mallocBehavior & TrackMemoryUsage) != 0;
}

typedef struct {
    void *memory;
    size_t bytes;
    bool didFree;
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
            realloc(allocationList.allocations, allocationList.capacity * 2);
        allocationList.capacity *= 2;
    }
    allocationList.allocations[allocationList.count++] =
        (Allocation){.memory = memory, .bytes = bytes};
}

static Allocation *
findAllocation(void *memory)
{
    for (size_t i = 0; i < allocationList.count; i++) {
        if (allocationList.allocations[i].memory == memory) {
            return &allocationList.allocations[i];
        }
    }
    return nullptr;
}

void
setMallocBehavior(MallocBehavior behavior)
{
    if (behavior & AlwaysFail) {
        behavior = AlwaysFail;
    }
    if ((behavior & LimitMemory) || (behavior & LogMemoryUsage)) {
        behavior |= TrackMemoryUsage;
    }
    mallocBehavior = behavior;
}

static size_t memoryLimit = 0;
static size_t allocatedMemory = 0;

void *
testMalloc(size_t bytes)
{
    if (alwaysFail()) {
        return nullptr;
    }
    if (limitMemory() && allocatedMemory + bytes > memoryLimit) {
        if (logMemoryUsage()) {
            fprintf(stderr, "Current %zu + %zu would exceed %zu\n",
                    allocatedMemory, bytes, memoryLimit);
        }
        errno = ENOMEM;
        return nullptr;
    }
    void *memory = malloc(bytes);
    if (memory == nullptr) {
        if (logMemoryUsage()) {
            fprintf(stderr, "Allocation of %zu bytes failed\n", bytes);
            perror("malloc");
        }
        return nullptr;
    }
    allocatedMemory += bytes;
    if (logMemoryUsage()) {
        fprintf(stderr, "Allocated %zu bytes at %p\n",
                bytes, memory);
    }
    if (trackMemoryUsage()) {
        addAllocation(memory, bytes);
    }
    return memory;
}

void *
testRealloc(void *memory, size_t bytes)
{
    if (memory == nullptr) {
        return testMalloc(bytes);
    }
    if (alwaysFail()) {
        return nullptr;
    }
    Allocation *alloc;
    if (trackMemoryUsage()) {
        alloc = findAllocation(memory);
        if (alloc == nullptr) {
            if (logMemoryUsage()) {
                fprintf(stderr, "Attempted to realloc untracked memory\n");
            }
            return nullptr;
        }
        if (alloc->didFree) {
            if (logMemoryUsage()) {
                fprintf(stderr, "Attempted to realloc freed memory %p\n",
                        memory);
            }
            return nullptr;
        }
    }
    if (limitMemory()) {
        if (allocatedMemory - alloc->bytes + bytes > memoryLimit) {
            if (logMemoryUsage()) {
                fprintf(stderr, "Current %zu + resize %p from %zu to %zu "
                        "would exceed limit %zu\n",
                        allocatedMemory, memory, alloc->bytes, bytes,
                        memoryLimit);
            }
            errno = ENOMEM;
            return nullptr;
        }
    }
    void *newMemory = realloc(memory, bytes);
    if (newMemory == nullptr) {
        if (logMemoryUsage()) {
            fprintf(stderr, "Allocation of %zu bytes failed\n", bytes);
            perror("realloc");
        }
        return nullptr;
    }
    if (logMemoryUsage()) {
        if (newMemory == alloc->memory) {
            fprintf(stderr, "Resized %p in place from %zu to %zu bytes\n",
                    newMemory,
                    alloc->bytes,
                    bytes);
        } else {
            fprintf(stderr, "%zu bytes at %p reallocated to %zu bytes at %p\n",
                    alloc->bytes,
                    alloc->memory,
                    bytes,
                    newMemory);
        }
    }
    if (trackMemoryUsage()) {
        allocatedMemory += bytes;
        allocatedMemory -= alloc->bytes;
        alloc->memory = newMemory;
        alloc->bytes = bytes;
    }
    return newMemory;
}

static unsigned doubleFreeAttempts = 0;

void
testFree(void *memory)
{
    if (alwaysFail()) {
        return;
    }
    Allocation *alloc;
    if (trackMemoryUsage()) {
        alloc = findAllocation(memory);
        if (alloc == nullptr) {
            if (logMemoryUsage()) {
                fprintf(stderr, "Attempt to free unknown memory at %p\n",
                        memory);
            }
            return;
        }
        if (alloc->didFree) {
            if (logMemoryUsage()) {
                fprintf(stderr, "Attempt to double-free memory at %p\n",
                        memory);
            }
            doubleFreeAttempts++;
            return;
        }
    }
    free(memory);
    if (logMemoryUsage()) {
        fprintf(stderr, "Freed %zu bytes at %p\n", alloc->bytes, memory);
    }
    if (trackMemoryUsage()) {
        alloc->didFree = true;
        allocatedMemory -= alloc->bytes;
    }
}

enum { MaxMemoryLimit = 1024 * 1024 * 1024 };

void
setMemoryLimit(size_t bytes)
{
    memoryLimit = bytes < MaxMemoryLimit ? bytes : MaxMemoryLimit;
}

void
resetAllocatedMemory()
{
    allocatedMemory = 0;
    allocationList.count = 0;
}

size_t
getAllocatedMemory()
{
    return allocatedMemory;
}

unsigned
getDoubleFreeAttempts()
{
    return doubleFreeAttempts;
}

void
resetDoubleFreeAttempts()
{
    doubleFreeAttempts = 0;
}

