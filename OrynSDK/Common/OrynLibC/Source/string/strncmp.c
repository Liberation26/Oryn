#include "string.h"

int strncmp(const char* left, const char* right, size_t size)
{
    while (size != 0U && *left != 0 && *left == *right)
    {
        ++left;
        ++right;
        --size;
    }
    return (size == 0U) ? 0 : ((unsigned char)*left - (unsigned char)*right);
}
