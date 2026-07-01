#include "stdlib.h"
#include "string.h"

void* bsearch(const void* key, const void* base, size_t count, size_t width,
    int (*compare)(const void*, const void*))
{
    size_t left = 0U;
    size_t right = count;
    const unsigned char* bytes = (const unsigned char*)base;
    while (left < right)
    {
        size_t middle = left + ((right - left) / 2U);
        const void* element = bytes + middle * width;
        int relation = compare(key, element);
        if (relation == 0) { return (void*)element; }
        if (relation < 0) { right = middle; }
        else { left = middle + 1U; }
    }
    return 0;
}

static void SwapBytes(unsigned char* left, unsigned char* right, size_t width)
{
    while (width-- > 0U)
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
    for (size_t i = 1U; i < count; ++i)
    {
        size_t j = i;
        while (j > 0U && compare(bytes + (j - 1U) * width, bytes + j * width) > 0)
        {
            SwapBytes(bytes + (j - 1U) * width, bytes + j * width, width);
            --j;
        }
    }
}
