#include "errno.h"
#include "limits.h"
#include "stdlib.h"

long strtol(const char* restrict text, char** restrict end, int base)
{
    unsigned long long parsed = strtoull(text, end, base);
    long value = (long)parsed;
    if (value == LONG_MAX || value == LONG_MIN)
    {
        errno = ERANGE;
    }
    return value;
}

int atoi(const char* text)
{
    return (int)strtol(text, 0, 10);
}
