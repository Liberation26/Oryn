#include "TargetBuildInternal.h"
#include <stdio.h>

int ReadStoredHash(const char* hash_file, unsigned long long* value)
{
    FILE* file = fopen(hash_file, "rb");
    if (file == 0)
    {
        return 0;
    }

    int ok = fscanf(file, "%llx", value) == 1;
    fclose(file);
    return ok;
}

void WriteStoredHash(const char* hash_file, unsigned long long value)
{
    FILE* file = fopen(hash_file, "wb");
    if (file == 0)
    {
        OrynLogWarn("Could not write object hash file.");
        return;
    }

    fprintf(file, "%016llX\n", value);
    fclose(file);
}
