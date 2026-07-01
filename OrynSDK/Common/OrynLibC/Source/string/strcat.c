#include "string.h"

char* strcat(char* restrict target, const char* restrict source)
{
    strcpy(target + strlen(target), source);
    return target;
}

char* strncat(char* restrict target, const char* restrict source, size_t size)
{
    char* output = target + strlen(target);
    size_t index = 0U;
    while (index < size && source[index] != 0)
    {
        output[index] = source[index];
        ++index;
    }
    output[index] = 0;
    return target;
}
