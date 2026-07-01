#include "string.h"

static int ContainsByte(const char* text, char value)
{
    while (*text != 0)
    {
        if (*text++ == value) { return 1; }
    }
    return 0;
}

size_t strcspn(const char* text, const char* reject)
{
    size_t count = 0U;
    while (text[count] != 0 && !ContainsByte(reject, text[count])) { ++count; }
    return count;
}
