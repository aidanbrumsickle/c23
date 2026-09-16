#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <stdint.h>

typedef enum : uint8_t {
    EntryState_Empty = 1,
    EntryState_Deleted = 2,
    EntryState_InUse = 4
} EntryState;

typedef enum {
    Map_Success,
    Map_NullPointer,
    Map_OutOfMemory,
    Map_IndexOutOfBounds
} MapStatus;

typedef struct {
    Str *keys;
    EntryState *state;
    void *values;
    Arena arena;
    uint32_t capacity;
    uint32_t count;
    uint32_t valueSize;
    uint32_t valueAlignment;
} StrMap;

typedef struct {
    StrMap *map;
    uint32_t index;
} MapIterator;

MapStatus
StrMap_initialize(StrMap *map);

uint32_t
StrMap_lookupNoCheck(StrMap *map, StrSlice key);

int64_t
StrMap_lookup(StrMap *map, StrSlice key);

bool
StrMap_indexInUse(StrMap *map, uint32_t index);

bool
StrMap_containsKey(StrMap *map, StrSlice key);

void *
StrMap_atIndex(StrMap *map, uint32_t index);

void *
StrMap_get(StrMap *map, StrSlice key);

MapStatus
StrMap_putAtIndexNoCheck(
        StrMap *map, uint32_t index, StrSlice key, void *value);

MapStatus
StrMap_putAtIndex(StrMap *map, uint32_t index, StrSlice key, void *value);

MapStatus
StrMap_putNoCheck(StrMap *map, StrSlice key, void *value);

MapStatus
StrMap_put(StrMap *map, StrSlice key, void *value);

void
StrMap_deleteAtIndexNoCheck(StrMap *map, uint32_t index);

MapStatus
StrMap_deleteAtIndex(StrMap *map, uint32_t index);

void
StrMap_deleteNoCheck(StrMap *map, StrSlice key);

MapStatus
StrMap_delete(StrMap *map, StrSlice key);

bool
StrMap_next(MapIterator *it, Str *key, void **value)

void
StrMap_free(StrMap *map);
#endif
