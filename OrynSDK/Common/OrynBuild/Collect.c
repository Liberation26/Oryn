#include "OrynBuild.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>

static int EndsWith(const char* text, const char* suffix)
{
    size_t text_length = strlen(text);
    size_t suffix_length = strlen(suffix);
    if (suffix_length > text_length)
    {
        return 0;
    }
    return strcmp(text + text_length - suffix_length, suffix) == 0;
}

static int CollectCFilesRecursive(const char* directory, OrynStringList* list)
{
    DIR* handle = opendir(directory);
    if (handle == 0)
    {
        return 0;
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
            CollectCFilesRecursive(child, list);
        }
        else if (OrynFileExists(child) && EndsWith(child, ".c"))
        {
            if (list->count >= ORYN_MAX_ITEMS)
            {
                closedir(handle);
                return 0;
            }
            snprintf(list->items[list->count], ORYN_MAX_PATH, "%s", child);
            list->count += 1;
        }
    }

    closedir(handle);
    return 1;
}

int OrynCollectCFiles(const char* directory, OrynStringList* list)
{
    list->count = 0;
    return CollectCFilesRecursive(directory, list);
}
