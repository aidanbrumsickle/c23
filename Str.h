#ifndef STR_H
#define STR_H
#include <stddef.h>
#include "Util.h"
#include "Arena.h"

// Does not carry its own memory
// Can slice within other slices or within Str.
// Not guaranteed to be nul-terminated.
typedef struct {
    const char *text;
    size_t length;
} StrSlice;

// Backed by its own memory
// Can be stack-based buffer, but usually heap-based.
// Guaranteed to be nul-terminated.
typedef struct {
    char *text;
    size_t length;
} Str;

typedef enum {
    LessThan = -1,
    EqualTo = 0,
    GreaterThan = 1
} Comparison;

// 
StrSlice
StrSlice_slice(StrSlice slice, size_t from, size_t to);

StrSlice
Str_slice(Str str, size_t from, size_t to);

StrSlice
Str_asSlice(Str str);

Comparison
StrSlice_compare(StrSlice s1, StrSlice s2);

// Generic comparison between Str and/or StrSlice.
// Arguments can be any combination of Str and StrSlice.
// Direct comparison with char* is not supported.
#define strCompare(s1, s2) \
    _Generic(s1, \
            StrSlice : _Generic(s2, \
                StrSlice : StrSlice_compare(s1, s2), \
                Str : StrSlice_compare(s1, Str_asSlice(s2))), \
            Str : _Generic(s2, \
                StrSlice : StrSlice_compare(Str_asSlice(s1), s2), \
                Str : StrSlice_compare( \
                    Str_asSlice(s1), Str_asSlice(s2)))) \

// TODO StrSlice_compareIgnoreCase(s1, s2)

// Returns a slice stopping at maxLength or the first NUL byte,
// whichever happens first
// If string is nullptr, returns a zero-width slice of nullptr
StrSlice
StrSlice_fromCStr(const char *string, size_t maxLength);

// Copy str using malloc.
// On allocation failure, returns {nullptr, 0}
Str
Str_copyStr(Str str);

// Copy str using malloc.
// On allocation failure, returns {nullptr, 0}
Str
Str_copyStrSlice(StrSlice slice);

// Copy str using malloc.
// On allocation failure, returns {nullptr, 0}
Str
Str_copyCStr(const char *string, size_t maxLength);

// Copy str using the arena.
// On allocation failure, returns {nullptr, 0}
Str
Str_arenaCopyStr(Str str, Arena *arena);

// Copy str using the arena.
// On allocation failure, returns {nullptr, 0}
Str
Str_arenaCopyStrSlice(StrSlice slice, Arena *arena);

// Copy str using the arena.
// On allocation failure, returns {nullptr, 0}
Str
Str_arenaCopyCStr(const char *string, size_t maxLength, Arena *arena);

// TODO StrSlice_strip(StrSlice slice)
// TODO StrSlice_stripChars(StrSlice slice, StrSlice chars);
// TODO StrSlice_stripLeading / stripTrailing
// TODO StrSlice_sliceUntil(StrSlice slice, StrSlice chars);
// TODO StrSlice_sliceAfter(StrSlice slice, StrSlice chars);
// TODO StrBuilder / StrBuffer structure - dynamic and fixed versions
// TODO StrSlice -> double, long
// TODO double, long -> Str
// TODO Str_readLine(FILE *f)
// TODO Str_readUntil(FILE *f, StrSlice stopChars);
// TODO Str_readFile(FILE *f, size_t maxLength);
// TODO StrSlice_writeToFile(StrSlice slice, FILE *f);
#endif
