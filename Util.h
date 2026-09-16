#ifndef UTIL_H
#define UTIL_H
#include <stdint.h>
#define min(a, b) _Generic((a), \
        signed char : minschar(a, b), \
        short : minshort(a, b), \
        int : minint(a, b), \
        long : minlong(a, b), \
        long long : minlonglong(a, b), \
        unsigned char : minuchar(a, b), \
        unsigned short : minushort(a, b), \
        unsigned int : minuint(a, b), \
        unsigned long : minulong(a, b), \
        unsigned long long : minulonglong(a, b))
#define max(a, b) _Generic((a), \
        signed char : maxschar(a, b), \
        short : maxshort(a, b), \
        int : maxint(a, b), \
        long : maxlong(a, b), \
        long long : maxlonglong(a, b), \
        unsigned char : maxuchar(a, b), \
        unsigned short : maxushort(a, b), \
        unsigned int : maxuint(a, b), \
        unsigned long : maxulong(a, b), \
        unsigned long long : maxulonglong(a, b))
#define defgeneric(name, body) \
    static inline signed char name##schar(signed char a, signed char b) { return body; } \
    static inline short name##short(short a, short b) { return body; } \
    static inline int name##int(int a, int b) { return body; } \
    static inline long name##long(long a, long b) { return body; } \
    static inline long long name##longlong(long long a, long long b) { return body; } \
    static inline unsigned char name##uchar(unsigned char a, unsigned char b) { return body; } \
    static inline unsigned short name##ushort(unsigned short a, unsigned short b) { return body; } \
    static inline unsigned int name##uint(unsigned int a, unsigned int b) { return body; } \
    static inline unsigned long name##ulong(unsigned long a, unsigned long b) { return body; } \
    static inline unsigned long long name##ulonglong(unsigned long long a, unsigned long long b) { return body; }
defgeneric(min, a < b ? a : b)
defgeneric(max, a > b ? a : b)
#endif
