#include "ctype.h"
#include "errno.h"
#include "limits.h"
#include "stdlib.h"

static int DigitValue(int value)
{
    if (value >= '0' && value <= '9') { return value - '0'; }
    if (value >= 'a' && value <= 'z') { return value - 'a' + 10; }
    if (value >= 'A' && value <= 'Z') { return value - 'A' + 10; }
    return -1;
}

static const char* SkipSpaceAndSign(const char* text, int* negative)
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

static int ResolveBase(const char** text, int base)
{
    if (base == 0)
    {
        if ((*text)[0] == '0' && ((*text)[1] == 'x' || (*text)[1] == 'X'))
        {
            *text += 2;
            return 16;
        }
        if ((*text)[0] == '0') { return 8; }
        return 10;
    }
    if (base == 16 && (*text)[0] == '0' && ((*text)[1] == 'x' || (*text)[1] == 'X'))
    {
        *text += 2;
    }
    return base;
}

unsigned long long strtoull(const char* restrict text, char** restrict end, int base)
{
    int negative = 0;
    const char* cursor = SkipSpaceAndSign(text, &negative);
    const char* first;
    unsigned long long value = 0ULL;
    base = ResolveBase(&cursor, base);
    first = cursor;
    if (base < 2 || base > 36) { errno = EINVAL; if (end) *end = (char*)text; return 0ULL; }
    for (;; ++cursor)
    {
        int digit = DigitValue((unsigned char)*cursor);
        if (digit < 0 || digit >= base) { break; }
        if (value > (ULLONG_MAX - (unsigned)digit) / (unsigned)base)
        {
            errno = ERANGE;
            value = ULLONG_MAX;
        }
        else if (value != ULLONG_MAX)
        {
            value = value * (unsigned)base + (unsigned)digit;
        }
    }
    if (end) { *end = (char*)((cursor == first) ? text : cursor); }
    return negative ? (0ULL - value) : value;
}
