#include "inttypes.h"
#include "stdlib.h"

intmax_t strtoimax(const char* restrict text, char** restrict end, int base)
{
    return (intmax_t)strtoll(text, end, base);
}
