#include "string.h"

void* memcpy(void* restrict target, const void* restrict source, size_t size)
{
    unsigned char* output = (unsigned char*)target;
    const unsigned char* input = (const unsigned char*)source;
    for (size_t index = 0U; index < size; ++index)
    {
        output[index] = input[index];
    }
    return target;
}
