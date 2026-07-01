#include "string.h"

static int ContainsByte(const char* text, char value)
{
    while (*text != 0)
    {
        if (*text == value) { return 1; }
        ++text;
    }
    return 0;
}

size_t strspn(const char* text, const char* accept)
{
    size_t count = 0U;
    while (text[count] != 0 && ContainsByte(accept, text[count]))
    {
        ++count;
    }
    return count;
}

size_t strcspn(const char* text, const char* reject)
{
    size_t count = 0U;
    while (text[count] != 0 && !ContainsByte(reject, text[count]))
    {
        ++count;
    }
    return count;
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
