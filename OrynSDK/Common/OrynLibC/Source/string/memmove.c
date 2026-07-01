#include "string.h"

void* memmove(void* target, const void* source, size_t size)
{
    unsigned char* output = (unsigned char*)target;
    const unsigned char* input = (const unsigned char*)source;
    if (output == input || size == 0U)
    {
        return target;
    }
    if (output < input)
    {
        for (size_t index = 0U; index < size; ++index)
        {
            output[index] = input[index];
        }
    }
    else
    {
        for (size_t index = size; index > 0U; --index)
        {
            output[index - 1U] = input[index - 1U];
        }
    }
    return target;
}
