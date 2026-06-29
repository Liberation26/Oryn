#include "OrynString.h"

void* OrynMemset(void* target, int value, size_t size)
{
    unsigned char* output = (unsigned char*)target;
    for (size_t index = 0; index < size; ++index)
    {
        output[index] = (unsigned char)value;
    }
    return target;
}

void* OrynMemcpy(void* target, const void* source, size_t size)
{
    unsigned char* output = (unsigned char*)target;
    const unsigned char* input = (const unsigned char*)source;
    for (size_t index = 0; index < size; ++index)
    {
        output[index] = input[index];
    }
    return target;
}

size_t OrynStrlen(const char* text)
{
    size_t length = 0;
    while (text[length] != 0)
    {
        ++length;
    }
    return length;
}
