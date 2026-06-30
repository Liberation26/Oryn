#include "TargetBuildInternal.h"
#include <string.h>

int EndsWithBuild(const char* text, const char* suffix)
{
    size_t text_length = strlen(text);
    size_t suffix_length = strlen(suffix);
    if (suffix_length > text_length)
    {
        return 0;
    }

    return strcmp(text + text_length - suffix_length, suffix) == 0;
}

int PathContainsBuild(const char* path, const char* needle)
{
    return strstr(path, needle) != 0;
}

int TextEqualsIgnoreCaseBuild(const char* left, const char* right)
{
    if (left == 0 || right == 0)
    {
        return 0;
    }

    while (*left != 0 && *right != 0)
    {
        char a = *left;
        char b = *right;
        if (a >= 'A' && a <= 'Z')
        {
            a = (char)(a - 'A' + 'a');
        }
        if (b >= 'A' && b <= 'Z')
        {
            b = (char)(b - 'A' + 'a');
        }
        if (a != b)
        {
            return 0;
        }
        ++left;
        ++right;
    }

    return *left == 0 && *right == 0;
}
