#include "string.h"

void* memchr(const void* memory, int value, size_t size)
{
    const unsigned char* input = (const unsigned char*)memory;
    unsigned char wanted = (unsigned char)value;
    for (size_t index = 0U; index < size; ++index)
    {
        if (input[index] == wanted)
        {
            return (void*)(input + index);
        }
    }
    return 0;
}
