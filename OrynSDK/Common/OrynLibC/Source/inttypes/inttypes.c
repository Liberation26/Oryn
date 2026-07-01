#include "inttypes.h"
#include "stdlib.h"

intmax_t strtoimax(const char* restrict text, char** restrict end, int base)
{
    return (intmax_t)strtoll(text, end, base);
}

uintmax_t strtoumax(const char* restrict text, char** restrict end, int base)
{
    return (uintmax_t)strtoull(text, end, base);
}

imaxdiv_t imaxdiv(intmax_t numerator, intmax_t denominator)
{
    imaxdiv_t result;
    result.quot = numerator / denominator;
    result.rem = numerator % denominator;
    return result;
}

intmax_t imaxabs(intmax_t value)
{
    return value < 0 ? -value : value;
}
