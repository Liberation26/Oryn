#include "TargetBuildInternal.h"
#include <stdio.h>
#include <string.h>

int IsLegacyProjectSharedSource(const OrynProject* project, const char* source_file)
{
    size_t root_length = strlen(project->source_dir);
    if (strncmp(source_file, project->source_dir, root_length) != 0)
    {
        return 0;
    }

    if (PathContainsBuild(source_file, "/Source/BootInfo/"))
    {
        return 1;
    }

    if (PathContainsBuild(source_file, "/Source/Console/KernelConsole.c") ||
        PathContainsBuild(source_file, "/Source/Fonts/") ||
        PathContainsBuild(source_file, "/Source/KernelIo.c") ||
        PathContainsBuild(source_file, "/Source/Serial.c"))
    {
        return 1;
    }

    if (PathContainsBuild(source_file, "/Source/Memory/KernelMemoryMap.c") ||
        PathContainsBuild(source_file, "/Source/Memory/KernelMemoryMapPrint.c") ||
        PathContainsBuild(source_file, "/Source/Memory/KernelPhysicalMemory.c") ||
        PathContainsBuild(source_file, "/Source/Memory/KernelPhysicalMemoryPrint.c") ||
        PathContainsBuild(source_file, "/Source/Memory/KernelVirtualMemory.c") ||
        PathContainsBuild(source_file, "/Source/Memory/KernelVirtualMemoryPrint.c"))
    {
        return 1;
    }

    return 0;
}

int AppendSourcesFromDirectory(const OrynProject* project, const char* directory, OrynStringList* sources, const char* label)
{
    if (directory[0] == 0 || !OrynDirectoryExists(directory))
    {
        return 1;
    }

    OrynStringList found;
    if (!OrynCollectCFiles(directory, &found))
    {
        char message[ORYN_MAX_PATH + 128];
        snprintf(message, sizeof(message), "Could not collect %s source files: %s", label, directory);
        OrynLogFail(message);
        return 0;
    }

    for (int index = 0; index < found.count; ++index)
    {
        if (IsLegacyProjectSharedSource(project, found.items[index]))
        {
            char message[ORYN_MAX_PATH + 128];
            snprintf(message, sizeof(message), "Ignored legacy project-owned SDK source: %s", found.items[index]);
            OrynLogWarn(message);
            continue;
        }

        if (sources->count >= ORYN_MAX_ITEMS)
        {
            OrynLogFail("Too many kernel source files were found.");
            return 0;
        }

        snprintf(sources->items[sources->count], ORYN_MAX_PATH, "%s", found.items[index]);
        sources->count += 1;
    }

    return 1;
}
