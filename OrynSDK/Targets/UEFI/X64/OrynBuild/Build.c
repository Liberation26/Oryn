#include "OrynBuild.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>


static unsigned long long OrynHashBytes(unsigned long long hash, const unsigned char* bytes, size_t count)
{
    for (size_t index = 0; index < count; ++index)
    {
        hash ^= (unsigned long long)bytes[index];
        hash *= 1099511628211ULL;
    }

    return hash;
}

static unsigned long long OrynHashText(unsigned long long hash, const char* text)
{
    return OrynHashBytes(hash, (const unsigned char*)text, strlen(text));
}

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

static int PathContains(const char* path, const char* needle)
{
    return strstr(path, needle) != 0;
}

static int OrynHashFile(unsigned long long* hash, const char* path)
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

static void OrynHashHeaderTreeRecursive(unsigned long long* hash, const char* directory)
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
        else if (OrynFileExists(child) && EndsWith(child, ".h"))
        {
            *hash = OrynHashText(*hash, "\nheader:\n");
            *hash = OrynHashText(*hash, child);
            *hash = OrynHashText(*hash, "\n");
            (void)OrynHashFile(hash, child);
        }
    }

    closedir(handle);
}

static void OrynHashHeaderTree(unsigned long long* hash, const char* directory)
{
    if (directory[0] == 0 || !OrynDirectoryExists(directory))
    {
        return;
    }

    OrynHashHeaderTreeRecursive(hash, directory);
}

static unsigned long long ComputeSourceBuildHash(const OrynProject* project, const char* source_file)
{
    unsigned long long hash = 1469598103934665603ULL;
    hash = OrynHashText(hash, ORYN_VERSION);
    hash = OrynHashText(hash, "\nsource:\n");
    (void)OrynHashFile(&hash, source_file);

    OrynHashHeaderTree(&hash, project->include_dir);
    OrynHashHeaderTree(&hash, project->sdk_kernel_common_include_dir);
    OrynHashHeaderTree(&hash, project->sdk_kernel_target_include_dir);

    if (project->selected_kernel_include_dir[0] != 0)
    {
        char selection_header[ORYN_MAX_PATH];
        OrynJoinPath(selection_header, sizeof(selection_header), project->selected_kernel_include_dir, "OrynBootInfoSelection.h");
        hash = OrynHashText(hash, "\nselection:\n");
        (void)OrynHashFile(&hash, selection_header);
    }

    return hash;
}

static int ReadStoredHash(const char* hash_file, unsigned long long* value)
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

static void WriteStoredHash(const char* hash_file, unsigned long long value)
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

static unsigned long long ComputePathHash(const char* path)
{
    unsigned long long hash = 1469598103934665603ULL;
    return OrynHashText(hash, path);
}

static void BuildObjectFileName(const OrynProject* project, const char* source_file, char* object_file, size_t object_file_size)
{
    char base_name[ORYN_MAX_PATH];
    char stem[ORYN_MAX_PATH];
    char object_name[ORYN_MAX_PATH];
    unsigned long long path_hash = ComputePathHash(source_file);

    OrynGetBaseName(base_name, sizeof(base_name), source_file);
    OrynReplaceExtension(stem, sizeof(stem), base_name, "");
    snprintf(object_name, sizeof(object_name), "%s-%016llX.o", stem, path_hash);
    OrynJoinPath(object_file, object_file_size, project->object_dir, object_name);
}

static int CompileSourceFile(const OrynProject* project, const char* source_file, char* object_file, size_t object_file_size)
{
    BuildObjectFileName(project, source_file, object_file, object_file_size);

    char hash_file[ORYN_MAX_PATH];
    snprintf(hash_file, sizeof(hash_file), "%s.hash", object_file);

    unsigned long long source_hash = ComputeSourceBuildHash(project, source_file);
    unsigned long long stored_hash = 0ULL;
    if (OrynFileExists(object_file) && ReadStoredHash(hash_file, &stored_hash) && stored_hash == source_hash)
    {
        char message[ORYN_MAX_PATH + 64];
        snprintf(message, sizeof(message), "Skipped unchanged source: %s", source_file);
        OrynLogOk(message);
        return 1;
    }

    char libc_include[ORYN_MAX_PATH];
    char handoff_include[ORYN_MAX_PATH];
    char target_boot_include[ORYN_MAX_PATH];
    OrynJoinPath(libc_include, sizeof(libc_include), project->sdk_root, "Common/OrynLibC/Include");
    OrynJoinPath(handoff_include, sizeof(handoff_include), project->sdk_root, "Common/Handoff/Include");
    OrynJoinPath(target_boot_include, sizeof(target_boot_include), project->sdk_root, "Targets/UEFI/X64/Boot/Include");

    char selection_include_argument[ORYN_MAX_PATH + 16];
    if (project->selected_kernel_include_dir[0] != 0)
    {
        snprintf(selection_include_argument, sizeof(selection_include_argument),
            "-I\"%s\" ", project->selected_kernel_include_dir);
    }
    else
    {
        selection_include_argument[0] = 0;
    }

    char command[ORYN_MAX_PATH * 9];
    snprintf(command, sizeof(command),
        "clang --target=x86_64-none-elf -ffreestanding -fno-stack-protector "
        "-fno-stack-check -fno-builtin -fno-pic -fno-pie -mno-red-zone -m64 "
        "-Wall -Wextra %s-I\"%s\" -I\"%s\" -I\"%s\" -I\"%s\" -I\"%s\" -I\"%s\" -c \"%s\" -o \"%s\"",
        selection_include_argument,
        project->sdk_kernel_common_include_dir,
        project->sdk_kernel_target_include_dir,
        project->include_dir,
        libc_include,
        handoff_include,
        target_boot_include,
        source_file,
        object_file);

    if (!OrynRunCommand(command))
    {
        return 0;
    }

    WriteStoredHash(hash_file, source_hash);
    return 1;
}

static int IsLegacyProjectSharedSource(const OrynProject* project, const char* source_file)
{
    size_t root_length = strlen(project->source_dir);
    if (strncmp(source_file, project->source_dir, root_length) != 0)
    {
        return 0;
    }

    if (PathContains(source_file, "/Source/BootInfo/"))
    {
        return 1;
    }

    if (PathContains(source_file, "/Source/Console/KernelConsole.c") ||
        PathContains(source_file, "/Source/Fonts/") ||
        PathContains(source_file, "/Source/KernelIo.c") ||
        PathContains(source_file, "/Source/Serial.c"))
    {
        return 1;
    }

    if (PathContains(source_file, "/Source/Memory/KernelMemoryMap.c") ||
        PathContains(source_file, "/Source/Memory/KernelMemoryMapPrint.c") ||
        PathContains(source_file, "/Source/Memory/KernelPhysicalMemory.c") ||
        PathContains(source_file, "/Source/Memory/KernelPhysicalMemoryPrint.c") ||
        PathContains(source_file, "/Source/Memory/KernelVirtualMemory.c") ||
        PathContains(source_file, "/Source/Memory/KernelVirtualMemoryPrint.c"))
    {
        return 1;
    }

    return 0;
}

static int AppendSourcesFromDirectory(const OrynProject* project, const char* directory, OrynStringList* sources, const char* label)
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

int OrynBuildKernel(const OrynProject* project)
{
    OrynLogStep("Building kernel.");
    if (project->selected_kernel_number > 0)
    {
        char message[ORYN_MAX_PATH + 128];
        snprintf(message, sizeof(message), "Using BootInfo variant Kernel/%d", project->selected_kernel_number);
        OrynLogOk(message);
    }
    else
    {
        OrynLogInfo("Using SDK default BootInfo selection because no Kernel/<number> variant is selected.");
    }

    OrynMakeDirectoryRecursive(project->object_dir);

    OrynStringList sources;
    sources.count = 0;

    if (!AppendSourcesFromDirectory(project, project->sdk_kernel_common_source_dir, &sources, "SDK common kernel") ||
        !AppendSourcesFromDirectory(project, project->sdk_kernel_target_source_dir, &sources, "SDK target kernel") ||
        !AppendSourcesFromDirectory(project, project->source_dir, &sources, "project kernel"))
    {
        return 0;
    }

    if (sources.count == 0)
    {
        OrynLogFail("No kernel source files were found.");
        return 0;
    }

    char message[256];
    snprintf(message, sizeof(message), "Kernel source units: %d", sources.count);
    OrynLogInfo(message);

    OrynStringList objects;
    objects.count = 0;

    for (int index = 0; index < sources.count; ++index)
    {
        if (!CompileSourceFile(project, sources.items[index], objects.items[objects.count], ORYN_MAX_PATH))
        {
            OrynLogFail("Source compile failed.");
            return 0;
        }
        objects.count += 1;
    }

    char linker_script[ORYN_MAX_PATH];
    OrynJoinPath(linker_script, sizeof(linker_script), project->sdk_root, "Targets/UEFI/X64/Kernel.ld");

    char kernel_elf[ORYN_MAX_PATH];
    OrynJoinPath(kernel_elf, sizeof(kernel_elf), project->build_dir, "Kernel.elf");

    char command[ORYN_MAX_PATH * 8];
    snprintf(command, sizeof(command),
        "ld.lld -nostdlib -static -z max-page-size=0x1000 -T \"%s\" -o \"%s\"",
        linker_script,
        kernel_elf);

    for (int index = 0; index < objects.count; ++index)
    {
        strncat(command, " \"", sizeof(command) - strlen(command) - 1);
        strncat(command, objects.items[index], sizeof(command) - strlen(command) - 1);
        strncat(command, "\"", sizeof(command) - strlen(command) - 1);
    }

    if (!OrynRunCommand(command))
    {
        OrynLogFail("Kernel link failed.");
        return 0;
    }

    OrynLogOk("Linked Build/Kernel.elf");
    return 1;
}
