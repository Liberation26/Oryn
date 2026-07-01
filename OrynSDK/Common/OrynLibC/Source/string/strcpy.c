#include "string.h"

char* strcpy(char* restrict target, const char* restrict source)
{
    char* output = target;
    while ((*output++ = *source++) != 0)
    {
    }
    return target;
}

char* strncpy(char* restrict target, const char* restrict source, size_t size)
{
    size_t index = 0U;
    for (; index < size && source[index] != 0; ++index)
    {
        target[index] = source[index];
    }
    for (; index < size; ++index)
    {
        target[index] = 0;
    }
    return target;
}
