#include "string.h"

void* memset(void* target, int value, size_t size)
{
    unsigned char* output = (unsigned char*)target;
    for (size_t index = 0U; index < size; ++index)
    {
        output[index] = (unsigned char)value;
    }
    return target;
}
