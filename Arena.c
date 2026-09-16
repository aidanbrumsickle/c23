#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include "Arena.h"
#include "Util.h"

#ifdef USE_TEST_MALLOC
#include "TestMalloc.h"
#define MALLOC_FN testMalloc
#define FREE_FN testFree
#else
#define MALLOC_FN malloc
#define FREE_FN free
#endif

typedef struct ArenaBlock {
    struct ArenaBlock *next;
    size_t capacity;
    size_t offset;
    unsigned char memory[];
} ArenaBlock;

constexpr size_t DefaultArenaCapacity = 64 * 1024L - sizeof(ArenaBlock);

// ASSUMES n > 0
// ENSURES returned value is smallest power of 2 >= n
static inline size_t
roundUpToPowerOfTwo(size_t n)
{
    size_t x = n - 1;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
}

// ENSURES returned value + sizeof(ArenaBlock) is smallest power of 2
//         >= capacity + sizeof(ArenaBlock)
static inline size_t adjustCapacity(size_t capacity)
{
    size_t totalSize = capacity + sizeof(ArenaBlock);
    size_t actualSize = roundUpToPowerOfTwo(totalSize);
    size_t actualCapacity = actualSize - sizeof(ArenaBlock);
    return actualCapacity;
}

// ASSUMES  capacity > 0
// REQUIRES sufficient system memory to allocate an ArenaBlock with capacity
//          extra bytes
// ENSURES  returned block != nullptr
// ENSURES  returned block->next == nullptr
// ENSURES  returned block->capacity == capacity
// ENSURES  returned block->offset == 0
static ArenaBlock *
ArenaBlock_newWithCapacity(size_t capacity)
{
    ArenaBlock *block = MALLOC_FN(sizeof(ArenaBlock) + capacity);
    if (!block) {
        return nullptr;
    }
    block->next = nullptr;
    block->capacity = capacity;
    block->offset = 0;
    // TODO should we zero-initialize blocks?
    return block;
}

static void
ArenaBlock_initializeInBuffer(void *buffer, size_t bufferSize)
{
    ArenaBlock *block = buffer;
    block->next = nullptr;
    block->capacity = bufferSize - sizeof(ArenaBlock);
    block->offset = 0;
}

// REQUIRES arena points to a memory region that can hold an Arena
// REQUIRES sufficient system memory to allocate an ArenaBlock with capacity
//          extra bytes
// ENSURES  arena is initialized
// ENSURES  at least capacity bytes can be allocated from arena without having
//          to allocate a new ArenaBlock from the backing allocator
ArenaStatusCode
Arena_initializeWithCapacity(Arena *arena, size_t capacity)
{
    if (!arena) {
        return Arena_NullPointer;
    }
    if (!capacity) {
        if (arena->maxCapacity) {
            capacity = min(arena->maxCapacity, DefaultArenaCapacity);
        } else {
            capacity = DefaultArenaCapacity;
        }
    }
    ArenaBlock *block = ArenaBlock_newWithCapacity(capacity);
    if (!block) {
        return Arena_OutOfMemory;
    }
    arena->head = block;
    return Arena_Success;
}

ArenaStatusCode
Arena_initialize(Arena *arena)
{
    return Arena_initializeWithCapacity(arena, DefaultArenaCapacity);
}

ArenaStatusCode
Arena_initializeFromStaticBuffer(Arena *arena, void *buffer, size_t bufferSize)
{
    if (!arena || !buffer) {
        return Arena_NullPointer;
    }
    ArenaBlock_initializeInBuffer(buffer, bufferSize);
    // buffer now contains a valid ArenaBlock
    ArenaBlock *block = buffer;
    arena->head = block;
    arena->maxCapacity = block->capacity;
    return Arena_Success;
}

size_t
Arena_maxAvailableWithoutBlockAllocation(Arena *arena)
{
    if (!arena) {
        return 0L;
    }
    size_t maxRemainingCapacity = 0;
    for (ArenaBlock *block = arena->head; block; block = block->next) {
        size_t remainingCapacity = block->capacity - block->offset;
        if (remainingCapacity > maxRemainingCapacity) {
            maxRemainingCapacity = remainingCapacity;
        }
    }
    return maxRemainingCapacity;
}

// alignment must be a power of two
ArenaAllocationResult
Arena_allocateAlignedWithoutBlockAllocation(
        Arena *arena, size_t bytes, size_t alignment)
{
    if (!arena) {
        return (ArenaAllocationResult){
            .memory = nullptr,
            .status = Arena_NullPointer
        };
    }
    for (ArenaBlock *block = arena->head; block; block = block->next) {
        size_t availableUnaligned = block->capacity - block->offset;
        uintptr_t unalignedAddress = (uintptr_t) &block->memory[block->offset];
        size_t mask = alignment - 1;
        size_t offsetFromAlignment = unalignedAddress & mask;
        size_t padding = (alignment - offsetFromAlignment) & mask;
        size_t available = availableUnaligned - padding;
        if (available < bytes) {
            continue;
        }
        size_t alignedBlockOffset = block->offset + padding;
        block->offset = alignedBlockOffset + bytes;
        return (ArenaAllocationResult){
            .memory = &block->memory[alignedBlockOffset],
            .status = Arena_Success
        };
    }
    return (ArenaAllocationResult){
        .memory = nullptr,
        .status = Arena_WouldRequireBlockAllocation
    };
}

ArenaAllocationResult
Arena_allocateWithoutBlockAllocation(Arena *arena, size_t bytes)
{
    return Arena_allocateAlignedWithoutBlockAllocation(arena, bytes, 1);
}

ArenaAllocationResult
Arena_allocateAligned(Arena *arena, size_t bytes, size_t alignment)
{
    if (!arena) {
        return (ArenaAllocationResult){
            .memory = nullptr,
            .status = Arena_NullPointer
        };
    }
    // Initialize if necessary
    if (!arena->head) {
        // Initialize with enough bytes if it does not exceed the maxCapacity
        size_t mask = alignment - 1;
        size_t padding =
            alignment - (offsetof(ArenaBlock, memory) & mask) & mask;
        bytes += padding;
        size_t capacity = adjustCapacity(bytes);
        size_t minCapacity = DefaultArenaCapacity;
        if (arena->maxCapacity) {
            if (capacity > arena->maxCapacity) {
                return (ArenaAllocationResult){
                    .memory = nullptr,
                    .status = Arena_WouldExceedMaxCapacity
                };
            }
            minCapacity = min(minCapacity, arena->maxCapacity);
        }
        capacity = max(capacity, minCapacity);
        ArenaStatusCode status = Arena_initializeWithCapacity(arena, capacity);
        if (status != Arena_Success) {
            return (ArenaAllocationResult){
                .memory = nullptr,
                .status = status
            };
        }
    }

    // Try to allocate without adding a new block
    ArenaAllocationResult result =
        Arena_allocateAlignedWithoutBlockAllocation(arena, bytes, alignment);
    if (result.status == Arena_Success) {
        return result;
    }

    // Calculate total capacity while finding tail of block list
    ArenaBlock *block = arena->head;
    size_t totalCapacity = block->capacity;
    while (block->next) {
        block = block->next;
        totalCapacity += block->capacity;
    }

    // Calculate total size required for new block
    // Do we need to add padding for extra alignment?
    size_t mask = alignment - 1;
    size_t padding = alignment - (offsetof(ArenaBlock, memory) & mask);
    bytes += padding;
    size_t nextBlockCapacity = adjustCapacity(max(bytes, 2 * block->capacity));
    // Enforce max capacity if configured
    if (arena->maxCapacity) {
        if (totalCapacity + nextBlockCapacity > arena->maxCapacity) {
            return (ArenaAllocationResult){
                .memory = nullptr,
                .status = Arena_WouldExceedMaxCapacity
            };
        }
    }

    ArenaBlock *nextBlock = ArenaBlock_newWithCapacity(nextBlockCapacity);
    if (!nextBlock) {
        return (ArenaAllocationResult){
            .memory = nullptr,
            .status = Arena_OutOfMemory
        };
    }
    block->next = nextBlock;
    nextBlock->offset += bytes;
    return (ArenaAllocationResult){
        .memory = &nextBlock->memory[padding],
        .status = Arena_Success
    };
}

ArenaAllocationResult
Arena_allocate(Arena *arena, size_t bytes)
{
    return Arena_allocateAligned(arena, bytes, 1);
}

// ASSUMES  buffer is large enough to fit sizeof(size_t) * (1 + block count)
// ENSURES  ArenaSnapshot is constructed at the start of buffer
// ENSURES  blockCount of constructed ArenaSnapshot is the current number
//          of blocks in the arena
// ENSURES  offsets of constructed ArenaSnapshot is of length blockCount
// ENSURES  offsets of constructed ArenaSnapshot contains offsets of each block
static void
Arena_takeSnapshotWithBufferNoCheck(Arena *arena, void *buffer)
{
    ArenaSnapshot *snapshot = buffer;
    snapshot->blockCount = 0;
    for (ArenaBlock *block = arena->head; block; block = block->next) {
        snapshot->offsets[snapshot->blockCount++] = block->offset;
    }
}

// ASSUMES  buffer fits at least size bytes
// REQUIRES size >= sizeof(size_t) * (1 + block count)
// ALWAYS   result.status = Arena_Success | Arena_InsufficientSnapshotBufferSize
// ENSURES  result.status = Arena_Success
// ENSURES  result.snapshot != nullptr
// ENSURES  result.snapshot == (ArenaSnapshot *)buffer
// ENSURES  result.snapshot->blockCount == current arena block count
// ENSURES  result.snapshot->offsets has length blockCount
// ENSURES  result.snapshot->offsets[i] == current offset of block i
//          for i in [0, blockCount)
ArenaSnapshotResult
Arena_takeSnapshotWithBuffer(Arena *arena, void *buffer, size_t size)
{
    // Check that buffer is big enough
    size_t requiredBufferSize = sizeof(size_t);
    for (ArenaBlock *block = arena->head; block; block = block->next) {
        requiredBufferSize += sizeof(size_t);
    }
    if (size < requiredBufferSize) {
        return (ArenaSnapshotResult){
            .snapshot = nullptr,
            .status = Arena_InsufficientSnapshotBufferSize
        };
    }
    Arena_takeSnapshotWithBufferNoCheck(arena, buffer);
    return (ArenaSnapshotResult){
        .snapshot = buffer,
        .status = Arena_Success
    };
}

ArenaSnapshotResult
Arena_takeSnapshot(Arena *arena)
{
    size_t requiredBufferSize = sizeof(size_t);
    for (ArenaBlock *block = arena->head; block; block = block->next) {
        requiredBufferSize += sizeof(size_t);
    }
    ArenaAllocationResult allocResult =
        Arena_allocateAligned(arena, requiredBufferSize, alignof(size_t));
    if (allocResult.status != Arena_Success) {
        return (ArenaSnapshotResult){
            .snapshot = nullptr,
            .status = allocResult.status
        };
    }
    ArenaSnapshot *snapshot = allocResult.memory;
    Arena_takeSnapshotWithBufferNoCheck(arena, snapshot);
    return (ArenaSnapshotResult){
        .snapshot = snapshot,
        .status = Arena_Success
    };
}

ArenaStatusCode
Arena_restoreSnapshot(Arena *arena, ArenaSnapshot *snapshot)
{
    if (!arena) {
        return Arena_NullPointer;
    }
    if (!snapshot) {
        return Arena_InvalidSnapshot;
    }
    // Validate snapshot before actually rolling back.
    // A snapshot is invalid if any of its offsets are out of bounds,
    // or if the snapshot has more blocks than the arena.
    ArenaBlock *block = arena->head;
    size_t i = 0;
    for (; block && i < snapshot->blockCount; block = block->next, i++) {
        if (snapshot->offsets[i] >= block->capacity) {
            return Arena_InvalidSnapshot;
        }
    }
    if (!block && i < snapshot->blockCount) {
        return Arena_InvalidSnapshot;
    }
    // Now actually apply the snapshot
    block = arena->head;
    i = 0;
    for (; block && i < snapshot->blockCount; block = block->next, i++) {
        block->offset = snapshot->offsets[i];
    }
    // Set all blocks newer than the snapshot to offset = 0
    for (; block; block = block->next) {
        block->offset = 0;
    }
    return Arena_Success;
}

// REQUIRES arena is a valid arena
// ENSURES all ArenaBlocks owned by the arena are freed
void
Arena_free(Arena *arena)
{
    if (!arena) {
        return;
    }
    ArenaBlock *block = arena->head;
    while (block) {
        ArenaBlock *next = block->next;
        FREE_FN(block);
        block = next;
    }
    arena->head = nullptr;
}
