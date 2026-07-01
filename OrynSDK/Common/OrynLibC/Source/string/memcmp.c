#include "string.h"

int memcmp(const void* left, const void* right, size_t size)
{
    const unsigned char* a = (const unsigned char*)left;
    const unsigned char* b = (const unsigned char*)right;
    for (size_t index = 0U; index < size; ++index)
    {
        if (a[index] != b[index])
        {
            return (int)a[index] - (int)b[index];
        }
    }
    return 0;
}
