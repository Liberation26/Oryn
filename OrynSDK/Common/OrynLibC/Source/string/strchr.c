#include "string.h"

char* strchr(const char* text, int value)
{
    char wanted = (char)value;
    for (;; ++text)
    {
        if (*text == wanted) { return (char*)text; }
        if (*text == 0) { return 0; }
    }
}
