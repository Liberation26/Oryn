#include "errno.h"
#include "limits.h"
#include "stdlib.h"

long strtol(const char* restrict text, char** restrict end, int base)
{
    long long value = strtoll(text, end, base);
    if (value > LONG_MAX) { errno = ERANGE; return LONG_MAX; }
    if (value < LONG_MIN) { errno = ERANGE; return LONG_MIN; }
    return (long)value;
}
