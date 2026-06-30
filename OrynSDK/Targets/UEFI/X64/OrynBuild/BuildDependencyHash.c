#include "TargetBuildInternal.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void OrynHashHeaderTreeRecursive(unsigned long long* hash, const char* directory)
{
    DIR* handle = opendir(directory);
    if (handle == 0)
    {
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(handle)) != 0)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char child[ORYN_MAX_PATH];
        OrynJoinPath(child, sizeof(child), directory, entry->d_name);

        if (OrynDirectoryExists(child))
        {
            OrynHashHeaderTreeRecursive(hash, child);
        }
        else if (OrynFileExists(child) && EndsWithBuild(child, ".h"))
        {
            *hash = OrynHashText(*hash, "\nheader:\n");
            *hash = OrynHashText(*hash, child);
            *hash = OrynHashText(*hash, "\n");
            (void)OrynHashFile(hash, child);
        }
    }

    closedir(handle);
}

void OrynHashHeaderTree(unsigned long long* hash, const char* directory)
{
    if (directory == 0 || directory[0] == 0 || !OrynDirectoryExists(directory))
    {
        return;
    }

    OrynHashHeaderTreeRecursive(hash, directory);
}

int HashDependencyToken(unsigned long long* hash, const char* token)
{
    if (token == 0 || token[0] == 0 || !OrynFileExists(token))
    {
        return 0;
    }

    *hash = OrynHashText(*hash, "\ndependency:\n");
    *hash = OrynHashText(*hash, token);
    *hash = OrynHashText(*hash, "\n");
    return OrynHashFile(hash, token);
}

int HashDependencyFile(unsigned long long* hash, const char* dependency_file)
{
    FILE* file = fopen(dependency_file, "rb");
    if (file == 0)
    {
        return 0;
    }

    char* buffer = (char*)malloc(1024U * 1024U);
    if (buffer == 0)
    {
        fclose(file);
        return 0;
    }

    size_t used = fread(buffer, 1, 1024U * 1024U - 1U, file);
    buffer[used] = 0;
    fclose(file);

    char* colon = strchr(buffer, ':');
    if (colon == 0)
    {
        free(buffer);
        return 0;
    }

    int hashed_any = 0;
    char token[ORYN_MAX_PATH];
    size_t token_used = 0;
    int escaping = 0;
    for (char* cursor = colon + 1; ; ++cursor)
    {
        char ch = *cursor;
        if (escaping && (ch == '\n' || ch == '\r'))
        {
            escaping = 0;
            continue;
        }
        if (escaping)
        {
            if (token_used + 1U < sizeof(token))
            {
                token[token_used++] = ch;
            }
            escaping = 0;
            continue;
        }
        if (ch == '\\')
        {
            escaping = 1;
            continue;
        }
        if (ch == 0 || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
        {
            if (token_used != 0U)
            {
                token[token_used] = 0;
                hashed_any |= HashDependencyToken(hash, token);
                token_used = 0U;
            }
            if (ch == 0)
            {
                break;
            }
            continue;
        }
        if (token_used + 1U < sizeof(token))
        {
            token[token_used++] = ch;
        }
    }

    free(buffer);
    return hashed_any;
}
