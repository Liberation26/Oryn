#include "string.h"

char* strstr(const char* haystack, const char* needle)
{
    if (*needle == 0)
    {
        return (char*)haystack;
    }
    for (; *haystack != 0; ++haystack)
    {
        const char* left = haystack;
        const char* right = needle;
        while (*left != 0 && *right != 0 && *left == *right)
        {
            ++left;
            ++right;
        }
        if (*right == 0)
        {
            return (char*)haystack;
        }
    }
    return 0;
}
