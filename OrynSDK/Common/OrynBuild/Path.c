#include "OrynBuild.h"
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int OrynPathExists(const char* path)
{
    struct stat status;
    return stat(path, &status) == 0;
}

int OrynFileExists(const char* path)
{
    struct stat status;
    if (stat(path, &status) != 0)
    {
        return 0;
    }
    return S_ISREG(status.st_mode);
}

int OrynDirectoryExists(const char* path)
{
    struct stat status;
    if (stat(path, &status) != 0)
    {
        return 0;
    }
    return S_ISDIR(status.st_mode);
}

void OrynJoinPath(char* output, size_t output_size, const char* left, const char* right)
{
    if (left == 0 || left[0] == 0)
    {
        snprintf(output, output_size, "%s", right);
        return;
    }

    size_t length = strlen(left);
    if (left[length - 1] == '/')
    {
        snprintf(output, output_size, "%s%s", left, right);
    }
    else
    {
        snprintf(output, output_size, "%s/%s", left, right);
    }
}

void OrynGetDirectoryName(char* output, size_t output_size, const char* path)
{
    snprintf(output, output_size, "%s", path);
    char* slash = strrchr(output, '/');
    if (slash == 0)
    {
        snprintf(output, output_size, ".");
        return;
    }
    if (slash == output)
    {
        slash[1] = 0;
        return;
    }
    *slash = 0;
}

void OrynGetBaseName(char* output, size_t output_size, const char* path)
{
    const char* slash = strrchr(path, '/');
    const char* name = slash == 0 ? path : slash + 1;
    snprintf(output, output_size, "%s", name);
}

void OrynReplaceExtension(char* output, size_t output_size, const char* path, const char* extension)
{
    snprintf(output, output_size, "%s", path);
    char* slash = strrchr(output, '/');
    char* dot = strrchr(output, '.');
    if (dot == 0 || (slash != 0 && dot < slash))
    {
        strncat(output, extension, output_size - strlen(output) - 1);
        return;
    }
    *dot = 0;
    strncat(output, extension, output_size - strlen(output) - 1);
}


void OrynMakeFatDirectoryName(char* output, size_t output_size, const char* input)
{
    size_t used = 0;
    if (output_size == 0)
    {
        return;
    }

    for (const char* current = input; *current != 0 && used + 1 < output_size && used < 8U; ++current)
    {
        char ch = *current;
        if (ch >= 'a' && ch <= 'z')
        {
            ch = (char)(ch - 'a' + 'A');
        }

        if ((ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '-' || ch == '_')
        {
            output[used++] = ch;
        }
    }

    if (used == 0)
    {
        snprintf(output, output_size, "KERNEL");
        return;
    }

    output[used] = 0;
}


void OrynMakeSafeFileBaseName(char* output, size_t output_size, const char* input)
{
    size_t used = 0;
    if (output_size == 0)
    {
        return;
    }

    for (const char* current = input; *current != 0 && used + 1 < output_size; ++current)
    {
        char ch = *current;
        if ((ch >= 'A' && ch <= 'Z') ||
            (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '-' || ch == '_')
        {
            output[used++] = ch;
        }
        else if (ch == ' ' || ch == '.')
        {
            if (used > 0 && output[used - 1] != '_')
            {
                output[used++] = '_';
            }
        }
    }

    while (used > 0 && output[used - 1] == '_')
    {
        --used;
    }

    if (used == 0)
    {
        snprintf(output, output_size, "Kernel");
        return;
    }

    output[used] = 0;
}

void OrynMakeKernelElfFileName(char* output, size_t output_size, const char* kernel_name)
{
    char base_name[256];
    OrynMakeSafeFileBaseName(base_name, sizeof(base_name), kernel_name);
    snprintf(output, output_size, "%s.elf", base_name);
}

void OrynNormalizePath(char* path)
{
    for (char* current = path; *current != 0; ++current)
    {
        if (*current == '\\')
        {
            *current = '/';
        }
    }
}

int OrynMakeDirectoryRecursive(const char* path)
{
    char partial[ORYN_MAX_PATH];
    snprintf(partial, sizeof(partial), "%s", path);
    OrynNormalizePath(partial);

    size_t length = strlen(partial);
    if (length == 0)
    {
        return 0;
    }

    if (partial[length - 1] == '/')
    {
        partial[length - 1] = 0;
    }

    for (char* current = partial + 1; *current != 0; ++current)
    {
        if (*current == '/')
        {
            *current = 0;
            if (mkdir(partial, 0755) != 0 && errno != EEXIST)
            {
                return 0;
            }
            *current = '/';
        }
    }

    if (mkdir(partial, 0755) != 0 && errno != EEXIST)
    {
        return 0;
    }

    return 1;
}

int OrynCopyFile(const char* source, const char* target)
{
    FILE* input = fopen(source, "rb");
    if (input == 0)
    {
        return 0;
    }

    char target_directory[ORYN_MAX_PATH];
    OrynGetDirectoryName(target_directory, sizeof(target_directory), target);
    OrynMakeDirectoryRecursive(target_directory);

    FILE* output = fopen(target, "wb");
    if (output == 0)
    {
        fclose(input);
        return 0;
    }

    char buffer[65536];
    size_t read_count;
    while ((read_count = fread(buffer, 1, sizeof(buffer), input)) > 0)
    {
        if (fwrite(buffer, 1, read_count, output) != read_count)
        {
            fclose(input);
            fclose(output);
            return 0;
        }
    }

    fclose(input);
    fclose(output);
    return 1;
}

int OrynRemoveDirectoryRecursive(const char* path)
{
    DIR* directory = opendir(path);
    if (directory == 0)
    {
        return 1;
    }

    struct dirent* entry;
    while ((entry = readdir(directory)) != 0)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char child[ORYN_MAX_PATH];
        OrynJoinPath(child, sizeof(child), path, entry->d_name);
        if (OrynDirectoryExists(child))
        {
            OrynRemoveDirectoryRecursive(child);
            rmdir(child);
        }
        else
        {
            unlink(child);
        }
    }

    closedir(directory);
    rmdir(path);
    return 1;
}

long long OrynFileModifiedTime(const char* path)
{
    struct stat status;
    if (stat(path, &status) != 0)
    {
        return 0;
    }
    return (long long)status.st_mtime;
}
