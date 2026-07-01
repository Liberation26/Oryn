#include "string.h"

char* strrchr(const char* text, int value)
{
    const char* found = 0;
    char wanted = (char)value;
    for (;; ++text)
    {
        if (*text == wanted) { found = text; }
        if (*text == 0) { break; }
    }
    return (char*)found;
}
