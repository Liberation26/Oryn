#include "string.h"

char* strncpy(char* restrict target, const char* restrict source, size_t size)
{
    char* start = target;
    while (size != 0U && *source != 0)
    {
        *target++ = *source++;
        --size;
    }
    while (size-- != 0U) { *target++ = 0; }
    return start;
}
