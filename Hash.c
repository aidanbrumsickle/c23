#include <stdint.h>
#include "Str.h"
#include "Hash.h"

static constexpr unsigned VectorLength = 4;
static constexpr unsigned KeyLength = 2;
static constexpr unsigned BlockLength = 8;
static constexpr unsigned LowByteMask = 0xff;
static constexpr uint64_t Initializer0 = 0x736f6d6570736575ULL;
static constexpr uint64_t Initializer1 = 0x646f72616e646f6dULL;
static constexpr uint64_t Initializer2 = 0x6c7967656e657261ULL;
static constexpr uint64_t Initializer3 = 0x7465646279746573ULL;

static inline uint64_t
rotl(uint64_t x, unsigned b)
{
    return (x << b) | (x >> (64 - b));
}

static inline void
sipRound(uint64_t v[static VectorLength])
{
    v[0] += v[1];
    v[2] += v[3];
    v[1] = rotl(v[1], 13);
    v[3] = rotl(v[3], 16);
    v[1] ^= v[0];
    v[3] ^= v[2];
    v[0] = rotl(v[0], 32);
    v[2] += v[1];
    v[0] += v[3];
    v[1] = rotl(v[1], 17);
    v[3] = rotl(v[3], 21);
    v[1] ^= v[2];
    v[3] ^= v[0];
    v[2] = rotl(v[2], 32);
}

static inline void
sipCompress2(uint64_t v[static VectorLength], const uint64_t m)
{
    v[3] ^= m;
    sipRound(v);
    sipRound(v);
    v[0] ^= m;
}

static inline void
sipFinalize4(uint64_t v[static VectorLength])
{
    v[2] ^= 0xff;
    sipRound(v);
    sipRound(v);
    sipRound(v);
    sipRound(v);
}

// Fall back to else clause on compilers that don't support __BYTE_ORDER__
// On MSVC that is little endian anyway
// Any compiler that does not define __BYTE_ORDER__ and targets a big endian
// system is unsupported.
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
static inline uint64_t
fullBlock(const uint8_t s[static BlockLength])
{
    return ((uint64_t)s[0] <<  0)
        |  ((uint64_t)s[1] <<  8)
        |  ((uint64_t)s[2] << 16)
        |  ((uint64_t)s[3] << 24)
        |  ((uint64_t)s[4] << 32)
        |  ((uint64_t)s[5] << 40)
        |  ((uint64_t)s[6] << 48)
        |  ((uint64_t)s[7] << 56);
}

static inline uint64_t
partialBlock(const uint8_t *s, size_t remainder, size_t lengthByte)
{
    size_t shift = 0;
    uint64_t value = 0ULL;
    for (size_t i = 0; i < remainder; i++) {
        value |= ((uint64_t)s[i] << shift);
        shift += 8;
    }
    value |= ((uint64_t)lengthByte << 56);
    return value;
}
#else
static inline uint64_t
fullBlock(const uint8_t s[static BlockLength])
{
    uint64_t result;
    memcpy(&result, s, sizeof(result));
    return result;
}

static inline uint64_t
partialBlock(const uint8_t *s, size_t remainder, size_t lengthByte)
{
    uint64_t result = 0;
    if (0 != remainder) {
        memcpy(&result, s, remainder);
    }
    result |= (uint64_t)lengthByte << 56;
    return result;
}
#endif

uint64_t
sipHash24(StrSlice slice, const uint64_t k[static KeyLength])
{
    uint64_t v[VectorLength];
    v[0] = k[0] ^ Initializer0;
    v[1] = k[1] ^ Initializer1;
    v[2] = k[0] ^ Initializer2;
    v[3] = k[1] ^ Initializer3;

    const size_t limit =
        s.length >= BlockLength ? s.length - BlockLength + 1 : 0;
    size_t offset;
    for (offset = 0; offset < limit; offset += BlockLength) {
        sipCompress2(v, fullBlock(slice.text + offset));
    }
    const uint8_t *s = slice.text + offset;
    size_t remainder = slice.length - offset;
    size_t lengthByte = slice.length & LowByteMask;
    sipCompress2(v, partialBlock(s, remainder, lengthByte));

    sipFinalize4(v);

    return v[0] ^ v[1] ^ v[2] ^ v[3];
}

#if defined(__APPLE__)
#include <stdlib.h>
void
randomizeKey(uint64_t k[static KeyLength])
{
    arc4random_buf(k, 128);
}
#elif defined(__linux__)
#include <sys/random.h>
#define NO_FLAGS 0
void
randomizeKey(uint64_t k[static KeyLength])
{
    char *p = (char *)k;
    size_t remaining = 128;
    while (remaining > 0) {
        // Should never require more than one iteration
        // since we're asking for less than 256 bytes,
        // but we will check just in case.
        ssize_t r = getrandom(p, remaining, NO_FLAGS);
        if (r < 0) { // error case
            if (errno == EINTR) {
                continue;
            }
            abort(); // All other errors are unrecoverable
        }
        p += r;
        remaining -= (size_t)r;
    }
}
#else
#error "Platform not supported"
#endif
