#include "errno.h"
#include "limits.h"
#include "stdlib.h"

unsigned long strtoul(const char* restrict text, char** restrict end, int base)
{
    unsigned long long value = strtoull(text, end, base);
    if (value > ULONG_MAX) { errno = ERANGE; return ULONG_MAX; }
    return (unsigned long)value;
}
