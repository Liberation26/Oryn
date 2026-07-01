#include "ctype.h"
#include "errno.h"
#include "limits.h"
#include "stdlib.h"

long long strtoll(const char* restrict text, char** restrict end, int base)
{
    const char* cursor = text;
    int negative = 0;
    unsigned long long limit;
    unsigned long long parsed;
    while (isspace((unsigned char)*cursor)) { ++cursor; }
    if (*cursor == '-' || *cursor == '+')
    {
        negative = (*cursor == '-') ? 1 : 0;
    }
    parsed = strtoull(text, end, base);
    limit = negative ? (unsigned long long)LLONG_MAX + 1ULL : (unsigned long long)LLONG_MAX;
    if (parsed > limit)
    {
        errno = ERANGE;
        return negative ? LLONG_MIN : LLONG_MAX;
    }
    return negative ? -(long long)parsed : (long long)parsed;
}

long strtol(const char* restrict text, char** restrict end, int base)
{
    long long value = strtoll(text, end, base);
    if (value > LONG_MAX) { errno = ERANGE; return LONG_MAX; }
    if (value < LONG_MIN) { errno = ERANGE; return LONG_MIN; }
    return (long)value;
}

int atoi(const char* text) { return (int)strtol(text, 0, 10); }
long atol(const char* text) { return strtol(text, 0, 10); }
long long atoll(const char* text) { return strtoll(text, 0, 10); }
