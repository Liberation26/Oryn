#include "TargetBuildInternal.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>

int ObjectListContainsBaseName(const OrynStringList* objects, const char* base_name)
{
    for (int index = 0; index < objects->count; ++index)
    {
        char current_base[ORYN_MAX_PATH];
        OrynGetBaseName(current_base, sizeof(current_base), objects->items[index]);
        if (strcmp(current_base, base_name) == 0)
        {
            return 1;
        }
    }

    return 0;
}

void RemoveSidecarIfPresent(const char* object_path, const char* extension)
{
    char sidecar[ORYN_MAX_PATH];
    BuildObjectSidecarPath(sidecar, sizeof(sidecar), object_path, extension);
    if (OrynFileExists(sidecar))
    {
        remove(sidecar);
    }
}

int SidecarListContainsBaseName(const OrynStringList* objects, const char* sidecar_base_name)
{
    for (int index = 0; index < objects->count; ++index)
    {
        char current_base[ORYN_MAX_PATH];
        char expected_hash[ORYN_MAX_PATH];
        char expected_dependency[ORYN_MAX_PATH];
        OrynGetBaseName(current_base, sizeof(current_base), objects->items[index]);
        snprintf(expected_hash, sizeof(expected_hash), "%s.hash", current_base);
        snprintf(expected_dependency, sizeof(expected_dependency), "%s.d", current_base);

        if (strcmp(expected_hash, sidecar_base_name) == 0 ||
            strcmp(expected_dependency, sidecar_base_name) == 0)
        {
            return 1;
        }
    }

    return 0;
}

int RemoveStaleSidecarFiles(const OrynProject* project, const OrynStringList* objects)
{
    DIR* directory = opendir(project->object_dir);
    if (directory == 0)
    {
        return 0;
    }

    int removed = 0;
    struct dirent* entry;
    while ((entry = readdir(directory)) != 0)
    {
        if (!EndsWithBuild(entry->d_name, ".o.hash") && !EndsWithBuild(entry->d_name, ".o.d"))
        {
            continue;
        }

        if (SidecarListContainsBaseName(objects, entry->d_name))
        {
            continue;
        }

        char sidecar_path[ORYN_MAX_PATH];
        OrynJoinPath(sidecar_path, sizeof(sidecar_path), project->object_dir, entry->d_name);
        if (remove(sidecar_path) == 0)
        {
            ++removed;
        }
    }

    closedir(directory);
    return removed;
}

int RemoveStaleObjectFiles(const OrynProject* project, const OrynStringList* objects)
{
    DIR* directory = opendir(project->object_dir);
    if (directory == 0)
    {
        return 0;
    }

    int removed = 0;
    struct dirent* entry;
    while ((entry = readdir(directory)) != 0)
    {
        if (!EndsWithBuild(entry->d_name, ".o"))
        {
            continue;
        }

        if (ObjectListContainsBaseName(objects, entry->d_name))
        {
            continue;
        }

        char object_path[ORYN_MAX_PATH];
        OrynJoinPath(object_path, sizeof(object_path), project->object_dir, entry->d_name);
        RemoveSidecarIfPresent(object_path, ".hash");
        RemoveSidecarIfPresent(object_path, ".d");
        if (remove(object_path) == 0)
        {
            ++removed;
        }
    }

    closedir(directory);
    removed += RemoveStaleSidecarFiles(project, objects);
    return removed;
}
