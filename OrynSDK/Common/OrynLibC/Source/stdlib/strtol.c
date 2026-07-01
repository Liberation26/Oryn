#include "ctype.h"
#include "errno.h"
#include "limits.h"
#include "stdlib.h"

static const char* OrynSkipSignedPrefix(const char* text, int* negative)
{
    while (isspace((unsigned char)*text)) { ++text; }
    *negative = 0;
    if (*text == '-' || *text == '+')
    {
        *negative = (*text == '-') ? 1 : 0;
        ++text;
    }
    return text;
}

static void OrynSetSignedEnd(char** restrict end, const char* original, char* magnitude_end)
{
    if (end == 0) { return; }
    *end = (magnitude_end == original) ? (char*)original : magnitude_end;
}

long long strtoll(const char* restrict text, char** restrict end, int base)
{
    int negative = 0;
    char* magnitude_end = 0;
    unsigned long long parsed;
    unsigned long long limit;
    const char* magnitude_text = OrynSkipSignedPrefix(text, &negative);

    errno = 0;
    parsed = strtoull(magnitude_text, &magnitude_end, base);

    if (magnitude_end == magnitude_text)
    {
        if (end != 0) { *end = (char*)text; }
        return 0LL;
    }

    OrynSetSignedEnd(end, text, magnitude_end);

    limit = negative ? ((unsigned long long)LLONG_MAX + 1ULL) : (unsigned long long)LLONG_MAX;
    if (parsed > limit)
    {
        errno = ERANGE;
        return negative ? LLONG_MIN : LLONG_MAX;
    }

    if (negative != 0)
    {
        if (parsed == ((unsigned long long)LLONG_MAX + 1ULL)) { return LLONG_MIN; }
        return -(long long)parsed;
    }

    return (long long)parsed;
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
