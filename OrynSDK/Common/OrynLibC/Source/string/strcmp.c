#include "string.h"

int strcmp(const char* left, const char* right)
{
    while (*left != 0 && *left == *right)
    {
        ++left;
        ++right;
    }
    return (int)(unsigned char)*left - (int)(unsigned char)*right;
}

int strncmp(const char* left, const char* right, size_t size)
{
    for (size_t index = 0U; index < size; ++index)
    {
        unsigned char a = (unsigned char)left[index];
        unsigned char b = (unsigned char)right[index];
        if (a != b || a == 0U || b == 0U)
        {
            return (int)a - (int)b;
        }
    }
    return 0;
}
