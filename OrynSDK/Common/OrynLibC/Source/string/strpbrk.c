#include "string.h"

static int ContainsByte(const char* text, char value)
{
    while (*text != 0)
    {
        if (*text++ == value) { return 1; }
    }
    return 0;
}

char* strpbrk(const char* text, const char* accept)
{
    while (*text != 0)
    {
        if (ContainsByte(accept, *text)) { return (char*)text; }
        ++text;
    }
    return 0;
}
