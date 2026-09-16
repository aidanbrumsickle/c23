#include <stdint.h>
// TODO extract headers for all of these
// need Str, Hash
#include "Arena.h"
#include "Str.h"
#include "Hash.h"
#include "HashMap.h"

constexpr int DefaultCapacity = 1024;
constexpr int DefaultAlignment = 8;

typedef enum : uint8_t {
    EntryState_Empty = 1,
    EntryState_Deleted = 2,
    EmptyState_InUse = 4
} EntryState;

typedef enum {
    Map_Success,
    Map_NullPointer,
    Map_OutOfMemory
} MapStatus;

typedef struct {
    Str *keys;
    EntryState *state;
    void *values;
    Arena arena;
    size_t capacity;
    size_t count;
    size_t valueSize;
    size_t valueAlignment;
} StrMap;

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

static MapStatus
StrMap_initializeExceptForArena(StrMap *map)
{
    if (!map) {
        return Map_NullPointer;
    }

    size_t capacity = map->capacity > 0 ?
        roundUpToPowerOfTwo(map->capacity) :
        DefaultCapacity;
    size_t keyBytes = capacity * sizeof(Str);
    size_t stateBytes = capacity * sizeof(EntryState);
    size_t valueBytes = capacity * init.valueSize;
    size_t alignment = map->valueAlignment ? map->valueAlignment :
        map->valueSize < DefaultAlignment ? map->valueSize : DefaultAlignment;
    size_t totalBytes = keyBytes + stateBytes + valueBytes;

    void *memory = aligned_alloc(alignment, totalBytes);
    if (!memory) {
        memset(map, 0, sizeof(StrMap));
        return Map_OutOfMemory;
    }

    map->keys = memory;
    map->state = (uint8_t*)map->keys + keyBytes;
    memset(map->state, EntryState_Empty, capacity);
    map->values = map->state + stateBytes;
    map->capacity = capacity;
    map->count = 0;
    map->valueAlignment = alignment;

    return Map_Success;
}

MapStatus
StrMap_initialize(StrMap *map)
{
    MapStatus status = StrMap_initializeExceptForArena(map);
    if (status != Map_Success) {
        return status;
    }
    ArenaStatusCode arenaStatus = Arena_initialize(&map->arena);
    if (status != Arena_Success) {
        free(map->keys);
        memset(map, 0, sizeof(StrMap));
        return Map_OutOfMemory;
    }
    return Map_Success;
}

static void
StrMap_setNoCheck(StrMap *map, int i, Str key, void *value)
{
    map->keys[i] = key;
    void *dest = (char *)map->values + i * map->valueSize;
    memcpy(dest, src, map->valueSize);
    map->state[i] = EntryState_InUse;
}

static uint64_t
hash(StrSlice slice)
{
    static bool didInitializeSipHashKey = false;
    static uint64_t k[2];
    if (!didInitializeSipHashKey) {
        didInitializeSipHashKey = true;
        randomizeKey(k);
    }
    return sipHash24(slice, k);
}

// Find the index in map for key by performing closed hashing
uint32_t
StrMap_lookupNoCheck(StrMap *map, StrSlice key)
{
    uint64_t keyHash = hash(key);
    const uint32_t capacityMask = map->capacity - 1;
    uint32_t index = keyHash & capacityMask;
    if (map->state[index] == EntryState_Empty
            || map->state[index] == EntryState_InUse
            && strCompare(key, map->keys[index]) == EqualTo) {
        return index;
    }
    bool sawTombstone = map->state[index] == EntryState_Deleted;
    uint32_t tombstoneIndex = sawTombstone ? index : 0;
    // Odd number less than capacity will be relatively prime to capacity
    // and thus should probe the entire capacity given enough iterations.
    uint32_t probeFactor = (uint32_t)((2 * (keyHash >> 32)) % capacityMask + 1);
    uint32_t probeCount = 1;
    do {
        index = (probeFactor * probeCount++) & capacityMask;
        if (!sawTombstone && map->state[index] == EntryState_Deleted) {
            sawTombstone = true;
            tombstoneIndex = index;
        }
    } while (map->state[index] == EntryState_Deleted
            || (map->state[index] == EntryState_InUse
                && strCompare(key, map->keys[index]) != EqualTo));
    if (sawTombstone && map->state[index] != EntryState_InUse) {
        return tombstoneIndex;
    }
    return index;
}

int64_t
StrMap_lookup(StrMap *map, StrSlice key)
{
    if (!map) {
        return -1;
    }
    return (int64_t)StrMap_lookupNoCheck(map, key);
}

bool
StrMap_indexInUse(StrMap *map, uint32_t index)
{
    return map
        && index < map->capacity
        && map->state[index] == EntryState_InUse;
}

bool
StrMap_containsKey(StrMap *map, StrSlice key)
{
    return map
        && map->state[StrMap_lookupNoCheck(map, key)] == EntryState_InUse;
}

static inline void *
StrMap_atIndexNoCheck(StrMap *map, uint32_t index)
{
    return (uint8_t *)map->values + index * map->valueSize;
}

void *
StrMap_atIndex(StrMap *map, uint32_t index)
{
    if (!StrMap_indexInUse(map, index)) {
        return nullptr;
    }
    return StrMap_atIndexNoCheck(map, index);
}

void *
StrMap_get(StrMap *map, StrSlice key)
{
    if (!map) {
        return nullptr;
    }
    uint32_t index = StrMap_lookupNoCheck(map, key);
    if (map->state[index] != EntryState_InUse) {
        return nullptr;
    }
    return StrMap_atIndexNoCheck(map, index);
}

static MapStatus
StrMap_grow(StrMap *map)
{
    if (!map) {
        return Map_NullPointer;
    }
    StrMap newMap = {
        .capacity = map->capacity * 2,
        .valueSize = map->valueSize,
        .valueAlignment = map->valueAlignment
    };
    MapStatus result = StrMap_initializeExceptForArena(&newMap);
    if (result != Map_Success) {
        return result;
    }
    for (uint32_t i = 0; i < map->capacity; i++) {
        if (map->state[i] != EntryState_InUse) {
            continue;
        }
        StrSlice slice = Str_asSlice(map->keys[i]);
        uint32_t newIndex = StrMap_lookupNoCheck(&newMap, slice);
        StrMap_setNoCheck(
                &newMap, newIndex, map->keys[i], StrMap_atIndex(map, i));
        newMap.count++;
    }
    free(map->keys);
    // Move arena
    newMap.arena = map->arena;
    *map = newMap;
    return Map_Success;
}

// ASSUMES index was returned by StrMap_lookup() AND map was not since resized
MapStatus
StrMap_putAtIndexNoCheck(StrMap *map, uint32_t index, StrSlice key, void *value)
{
    Str ownKey;
    if (map->state[index] != EntryState_InUse) {
        ownKey = Str_arenaCopyStrSlice(key, &map->arena);
        if (!ownKey.text) {
            // copy failed
            return Map_OutOfMemory;
        }
        map->count++;
    } else {
        ownKey = map->keys[index];
    }
    StrMap_setNoCheck(map, index, ownKey, value);
    if (map->count * 2 < map->capacity) {
        return Map_Success;
    }
    return StrMap_grow(map);
}

MapStatus
StrMap_putAtIndex(StrMap *map, uint32_t index, StrSlice key, void *value)
{
    if (!map || !value) {
        return Map_NullPointer;
    }
    if (index >= map->capacity) {
        return Map_IndexOutOfBounds;
    }
    return StrMap_putAtIndexNoCheck(map, index, key, value);
}

MapStatus
StrMap_putNoCheck(StrMap *map, StrSlice key, void *value)
{
    uint32_t index = StrMap_lookupNoCheck(map, key);
    return StrMap_putAtIndexNoCheck(map, index, key, value);
}

MapStatus
StrMap_put(StrMap *map, StrSlice key, void *value)
{
    if (!map) {
        return Map_NullPointer;
    }
    return StrMap_putNoCheck(map, key, value);
}

void
StrMap_deleteAtIndexNoCheck(StrMap *map, uint32_t index)
{
    map->state[index] = EntryState_Deleted;
}

MapStatus
StrMap_deleteAtIndex(StrMap *map, uint32_t index)
{
    if (!map) {
        return Map_NullPointer;
    }
    if (index >= map->capacity) {
        return Map_IndexOutOfBounds;
    }
    if (map->state[index] == EntryState_InUse) {
        StrMap_deleteAtIndexNoCheck(map, index);
    }
    return Map_Success;
}

void
StrMap_deleteNoCheck(StrMap *map, StrSlice key)
{
    map->state[StrMap_lookupNoCheck(map, key)] = EntryState_Deleted;
}

MapStatus
StrMap_delete(StrMap *map, StrSlice key)
{
    if (!map) {
        return Map_NullPointer;
    }
    uint32_t index = StrMap_lookupNoCheck(map, key);
    if (map->state[index] == EntryState_InUse) {
        StrMap_deleteAtIndexNoCheck(map, index);
    }
    return Map_Success;
}

bool
StrMap_next(MapIterator *it, Str *key, void **value)
{
    bool next = false;
    for (; !next && it && it->index < it->map->capacity; it->index++) {
        if (it->map->state[it->index] == EntryState_InUse) {
            *key = it->map->keys[it->index];
            *value = (char *)it->map->values + it->index * it->map->valueSize;
            next = true;
        }
    }
    return next;
}

void
StrMap_free(StrMap *map)
{
    free(map->keys);
    Arena_free(&map->arena);
}
