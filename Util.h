#ifndef UTIL_H
#define UTIL_H
#include <stdint.h>
#define min(a, b) _Generic((a), \
        int8_t : minint8_t(a, b), \
        int16_t : minint16_t(a, b), \
        int32_t : minint32_t(a, b), \
        int64_t : minint64_t(a, b), \
        uint8_t : minuint8_t(a, b), \
        uint16_t : minuint16_t(a, b), \
        uint32_t : minuint32_t(a, b), \
        uint64_t : minuint64_t(a, b))
#define max(a, b) _Generic((a), \
        int8_t : maxint8_t(a, b), \
        int16_t : maxint16_t(a, b), \
        int32_t : maxint32_t(a, b), \
        int64_t : maxint64_t(a, b), \
        uint8_t : maxuint8_t(a, b), \
        uint16_t : maxuint16_t(a, b), \
        uint32_t : maxuint32_t(a, b), \
        uint64_t : maxuint64_t(a, b))
#define defgeneric(name, body) \
    static inline int8_t name##int8_t(int8_t a, int8_t b) { return body; } \
    static inline int16_t name##int16_t(int16_t a, int16_t b) { return body; } \
    static inline int32_t name##int32_t(int32_t a, int32_t b) { return body; } \
    static inline int64_t name##int64_t(int64_t a, int64_t b) { return body; } \
    static inline uint8_t name##uint8_t(uint8_t a, uint8_t b) { return body; } \
    static inline uint16_t name##uint16_t(uint16_t a, uint16_t b) { return body; } \
    static inline uint32_t name##uint32_t(uint32_t a, uint32_t b) { return body; } \
    static inline uint64_t name##uint64_t(uint64_t a, uint64_t b) { return body; }
defgeneric(min, a < b ? a : b)
defgeneric(max, a > b ? a : b)
#endif
