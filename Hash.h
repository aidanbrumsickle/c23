#ifndef HASH_H
#define HASH_H
#include <stdint.h>
#include "Str.h"
uint64_t
sipHash24(StrSlice slice, const uint64_t k[static 2]);
void
randomizeKey(uint64_t k[static 2]);
#endif
