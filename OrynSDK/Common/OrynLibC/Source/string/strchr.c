#include "string.h"

char* strchr(const char* text, int value)
{
    char wanted = (char)value;
    for (;;)
    {
        if (*text == wanted)
        {
            return (char*)text;
        }
        if (*text == 0)
        {
            return 0;
        }
        ++text;
    }
}

char* strrchr(const char* text, int value)
{
    const char* found = 0;
    char wanted = (char)value;
    for (;;)
    {
        if (*text == wanted)
        {
            found = text;
        }
        if (*text == 0)
        {
            return (char*)found;
        }
        ++text;
    }
}
