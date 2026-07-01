#include "string.h"

char* strncat(char* restrict target, const char* restrict source, size_t size)
{
    char* end = target + strlen(target);
    while (size-- != 0U && *source != 0) { *end++ = *source++; }
    *end = 0;
    return target;
}
