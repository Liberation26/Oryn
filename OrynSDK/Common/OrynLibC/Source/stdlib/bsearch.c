#include "stdlib.h"

void* bsearch(const void* key, const void* base, size_t count, size_t width,
    int (*compare)(const void*, const void*))
{
    const unsigned char* bytes = (const unsigned char*)base;
    size_t low = 0U;
    size_t high = count;
    while (low < high)
    {
        size_t mid = low + ((high - low) / 2U);
        const void* item = bytes + (mid * width);
        int cmp = compare(key, item);
        if (cmp == 0) { return (void*)item; }
        if (cmp < 0) { high = mid; } else { low = mid + 1U; }
    }
    return 0;
}
