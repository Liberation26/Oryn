#include "TargetBuildInternal.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>

static int EndsWithCSource(const char* text)
{
    size_t length = strlen(text);
    return length >= 2U && strcmp(text + length - 2U, ".c") == 0;
}

static void LogSourceDecision(const OrynProject* project, const char* module_name, const char* decision, const char* path)
{
    char detail[ORYN_MAX_PATH + 256];
    snprintf(detail, sizeof(detail), "module=%s path=%s", module_name, path);
    LogBuildPlanDecision(project, decision, detail);
}

static void LogSourceSkip(const OrynProject* project, const char* module_name, const char* decision, const char* path)
{
    char detail[ORYN_MAX_PATH + 256];
    snprintf(detail, sizeof(detail), "module=%s path=%s", module_name, path);
    LogBuildPlanSkip(project, decision, detail);
}

static int CollectCFilesModeRecursive(const OrynProject* project, const char* module_name, const char* directory, int recursive, OrynStringList* list)
{
    DIR* handle = opendir(directory);
    if (handle == 0)
    {
        LogSourceSkip(project, module_name, "source-directory-open-failed", directory);
        return 0;
    }

    LogSourceDecision(project, module_name, "source-directory-enter", directory);

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
            if (recursive)
            {
                LogSourceDecision(project, module_name, "source-directory-recursing", child);
                if (!CollectCFilesModeRecursive(project, module_name, child, recursive, list))
                {
                    closedir(handle);
                    return 0;
                }
            }
            else
            {
                LogSourceSkip(project, module_name, "source-directory-skipped-non-recursive", child);
            }
        }
        else if (OrynFileExists(child) && EndsWithCSource(child))
        {
            if (list->count >= ORYN_MAX_ITEMS)
            {
                LogSourceSkip(project, module_name, "source-skipped-list-full", child);
                closedir(handle);
                return 0;
            }
            snprintf(list->items[list->count], ORYN_MAX_PATH, "%s", child);
            list->count += 1;
            LogSourceDecision(project, module_name, "source-included-c", child);
        }
        else if (OrynFileExists(child))
        {
            LogSourceSkip(project, module_name, "source-skipped-not-c", child);
        }
        else
        {
            LogSourceSkip(project, module_name, "source-skipped-not-file-or-directory", child);
        }
    }

    closedir(handle);
    return 1;
}

int CollectCFilesFromDirectoryMode(const OrynProject* project, const char* module_name, const char* directory, int recursive, OrynStringList* list)
{
    list->count = 0;
    if (directory[0] == 0)
    {
        LogBuildPlanSkip(project, "module-source-root-empty", module_name);
        return 1;
    }

    if (!OrynDirectoryExists(directory))
    {
        char detail[ORYN_MAX_PATH + 160];
        snprintf(detail, sizeof(detail), "module=%s source-root=%s", module_name, directory);
        LogBuildPlanSkip(project, "module-source-root-not-present", detail);
        return 1;
    }

    return CollectCFilesModeRecursive(project, module_name, directory, recursive, list);
}
