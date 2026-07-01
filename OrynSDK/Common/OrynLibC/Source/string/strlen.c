#include "string.h"

size_t strlen(const char* text)
{
    const char* start = text;
    while (*text != 0) { ++text; }
    return (size_t)(text - start);
}
