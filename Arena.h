#ifndef ARENA_H
#define ARENA_H
#include <stdint.h>

struct ArenaBlock;

typedef struct {
    struct ArenaBlock *head;
    size_t maxCapacity; // Enforced if > 0
} Arena;

typedef enum {
    Arena_Success,
    Arena_NullPointer,
    Arena_Uninitialized,
    Arena_OutOfMemory,
    Arena_WouldRequireBlockAllocation,
    Arena_WouldExceedMaxCapacity,
    Arena_InsufficientSnapshotBufferSize,
    Arena_InvalidSnapshot
} ArenaStatusCode;

typedef struct {
    void *memory;
    ArenaStatusCode status;
} ArenaAllocationResult;

typedef struct {
    size_t blockCount;
    size_t offsets[];
} ArenaSnapshot;

typedef struct {
    ArenaSnapshot *snapshot;
    ArenaStatusCode status;
} ArenaSnapshotResult;

ArenaStatusCode
Arena_initializeWithCapacity(Arena *arena, size_t capacity);

ArenaStatusCode
Arena_initialize(Arena *arena);

ArenaStatusCode
Arena_initializeFromStaticBuffer(Arena *arena, void *buffer, size_t bufferSize);

size_t
Arena_maxAvailableWithoutBlockAllocation(Arena *arena);

ArenaAllocationResult
Arena_allocateAlignedWithoutBlockAllocation(
        Arena *arena, size_t bytes, size_t alignment);

ArenaAllocationResult
Arena_allocateWithoutBlockAllocation(Arena *arena, size_t bytes);

ArenaAllocationResult
Arena_allocateAligned(Arena *arena, size_t bytes, size_t alignment);

ArenaAllocationResult
Arena_allocate(Arena *arena, size_t bytes);

ArenaSnapshotResult
Arena_takeSnapshotWithBuffer(Arena *arena, void *buffer, size_t size);

ArenaSnapshotResult
Arena_takeSnapshot(Arena *arena);

ArenaStatusCode
Arena_restoreSnapshot(Arena *arena, ArenaSnapshot *snapshot);

void
Arena_free(Arena *arena);
#endif
