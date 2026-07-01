#include "stdlib.h"

static void SwapBytes(unsigned char* left, unsigned char* right, size_t width)
{
    while (width-- != 0U)
    {
        unsigned char temp = *left;
        *left++ = *right;
        *right++ = temp;
    }
}

void qsort(void* base, size_t count, size_t width,
    int (*compare)(const void*, const void*))
{
    unsigned char* bytes = (unsigned char*)base;
    size_t i;
    for (i = 1U; i < count; ++i)
    {
        size_t j = i;
        while (j > 0U && compare(bytes + (j * width), bytes + ((j - 1U) * width)) < 0)
        {
            SwapBytes(bytes + (j * width), bytes + ((j - 1U) * width), width);
            --j;
        }
    }
}
