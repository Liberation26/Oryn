#include "inttypes.h"
#include "stdlib.h"

uintmax_t strtoumax(const char* restrict text, char** restrict end, int base)
{
    return (uintmax_t)strtoull(text, end, base);
}
