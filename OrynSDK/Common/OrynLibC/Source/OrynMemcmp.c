#include "OrynString.h"

int OrynMemcmp(const void* left, const void* right, size_t size)
{
    return memcmp(left, right, size);
}
