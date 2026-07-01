#ifndef ORYN_LIBC_INTTYPES_H
#define ORYN_LIBC_INTTYPES_H

#include "stdint.h"

#define PRId8 "d"
#define PRId16 "d"
#define PRId32 "d"
#define PRId64 "lld"
#define PRIi8 "i"
#define PRIi16 "i"
#define PRIi32 "i"
#define PRIi64 "lli"
#define PRIu8 "u"
#define PRIu16 "u"
#define PRIu32 "u"
#define PRIu64 "llu"
#define PRIo8 "o"
#define PRIo16 "o"
#define PRIo32 "o"
#define PRIo64 "llo"
#define PRIx8 "x"
#define PRIx16 "x"
#define PRIx32 "x"
#define PRIx64 "llx"
#define PRIX8 "X"
#define PRIX16 "X"
#define PRIX32 "X"
#define PRIX64 "llX"
#define PRIdMAX "lld"
#define PRIiMAX "lli"
#define PRIuMAX "llu"
#define PRIxMAX "llx"
#define PRIXMAX "llX"
#define SCNdMAX "lld"
#define SCNiMAX "lli"
#define SCNuMAX "llu"
#define SCNxMAX "llx"

typedef struct { intmax_t quot; intmax_t rem; } imaxdiv_t;

intmax_t strtoimax(const char* restrict text, char** restrict end, int base);
uintmax_t strtoumax(const char* restrict text, char** restrict end, int base);
imaxdiv_t imaxdiv(intmax_t numerator, intmax_t denominator);
intmax_t imaxabs(intmax_t value);

#endif
