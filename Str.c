#include <string.h>
#include "Str.h"
#include "Util.h"

StrSlice
StrSlice_slice(StrSlice slice, size_t from, size_t to)
{
    if (slice.length == 0) {
        return slice;
    }
    from = min(from, slice.length - 1);
    to = min(to, slice.length);
    return (StrSlice){slice.text + from, to - from};
}

StrSlice
Str_slice(Str str, size_t from, size_t to)
{
    if (str.length == 0) {
        return str;
    }
    from = min(from, str.length - 1);
    to = min(to, str.length);
    return (StrSlice){(const char *)str.text + from, to - from};
}

StrSlice
Str_asSlice(Str str)
{
    return (StrSlice){(const char *)str.text, str.length};
}

Comparison
StrSlice_compare(StrSlice s1, StrSlice s2)
{
    if (s1.text == s2.text && s1.length == s2.length) {
        return EqualTo;
    }
    size_t length = min(s1.length, s2.length);
    for (size_t i = 0; i < length; i++) {
        if (s1[i] < s2[i]) {
            return LessThan;
        }
        if (s1[i] > s2[i]) {
            return GreaterThan;
        }
    }
    if (s1.length < s2.length) {
        return LessThan;
    }
    if (s1.length > s2.length) {
        return GreaterThan;
    }
    return EqualTo;
}

// Returns a slice stopping at maxLength or the first NUL byte,
// whichever happens first
StrSlice
StrSlice_fromCStr(const char *string, size_t maxLength)
{
    if (!nulTerminatedString) {
        return (StrSlice){nullptr, 0};
    }
    size_t length = strnlen(nulTerminatedString);
    return (StrSlice){nulTerminatedString, length};
}

// Copy str using malloc.
// On allocation failure, returns {nullptr, 0}
Str
Str_copyStr(Str str)
{
    char *textCopy = malloc(str.length + 1);
    if (!textCopy) {
        return (Str){nullptr, 0};
    }
    memcpy(textCopy, str.text, str.length);
    textCopy[str.length] = '\0';
    return (Str){textCopy, str.length};
}

// Copy slice as Str adding nul terminator using malloc.
// On allocation failure, returns {nullptr, 0}
Str
Str_copyStrSlice(StrSlice slice)
{
    char *textCopy = malloc(slice.length + 1);
    if (!textCopy) {
        return (Str){nullptr, 0};
    }
    memcpy(textCopy, slice.text, slice.length);
    textCopy[slice.length] = '\0';
    return (Str){textCopy, slice.length};
}

// Copy C string as Str using strndup
// On allocation failure, returns {nullptr, 0}
Str
Str_copyCStr(const char *string, size_t maxLength)
{
    size_t length = strnlen(string, maxLength);
    // length must be <= maxLength
    char *textCopy = strndup(string, length);
    if (!textCopy) {
        return (Str){nullptr, 0};
    }
    return (Str){textCopy, length};
}

// Copy str using the arena.
// On allocation failure, returns {nullptr, 0}
Str
Str_arenaCopyStr(Str str, Arena *arena)
{
    ArenaAllocationResult result = Arena_allocate(arena, str.length + 1);
    if (result != Arena_Success) {
        return (Str){nullptr, 0};
    }
    char *textCopy = result.memory;
    memcpy(textCopy, str.text, str.length);
    textCopy[str.length] = '\0';
    return (Str){textCopy, str.length};
}

// Copy str using the arena.
// On allocation failure, returns {nullptr, 0}
Str
Str_arenaCopyStrSlice(StrSlice slice, Arena *arena)
{
    ArenaAllocationResult result = Arena_allocate(arena, slice.length + 1);
    if (result != Arena_Success) {
        return (Str){nullptr, 0};
    }
    char *textCopy = result.memory;
    memcpy(textCopy, slice.text, slice.length);
    textCopy[slice.length] = '\0';
    return (Str){textCopy, slice.length};
}

// Copy str using the arena.
// On allocation failure, returns {nullptr, 0}
Str
Str_arenaCopyCStr(const char *string, size_t maxLength, Arena *arena)
{
    size_t length = strnlen(string, maxLength);
    ArenaAllocationResult result = Arena_allocate(arena, length + 1);
    if (result != Arena_Success) {
        return (Str){nullptr, 0};
    }
    char *textCopy = result.memory;
    memcpy(textCopy, string, length);
    textCopy[length] = '\0';
    return (Str){textCopy, length};
}
