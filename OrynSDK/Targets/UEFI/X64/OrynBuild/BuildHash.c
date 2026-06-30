#include "TargetBuildInternal.h"
#include <stdio.h>
#include <string.h>

unsigned long long OrynHashBytes(unsigned long long hash, const unsigned char* bytes, size_t count)
{
    for (size_t index = 0; index < count; ++index)
    {
        hash ^= (unsigned long long)bytes[index];
        hash *= 1099511628211ULL;
    }

    return hash;
}

unsigned long long OrynHashText(unsigned long long hash, const char* text)
{
    if (text == 0)
    {
        return OrynHashText(hash, "<null>");
    }

    return OrynHashBytes(hash, (const unsigned char*)text, strlen(text));
}

int OrynHashFile(unsigned long long* hash, const char* path)
{
    FILE* file = fopen(path, "rb");
    if (file == 0)
    {
        return 0;
    }

    unsigned char buffer[4096];
    size_t count;
    while ((count = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        *hash = OrynHashBytes(*hash, buffer, count);
    }

    fclose(file);
    return 1;
}

unsigned long long ComputePathHash(const char* path)
{
    unsigned long long hash = 1469598103934665603ULL;
    return OrynHashText(hash, path);
}
