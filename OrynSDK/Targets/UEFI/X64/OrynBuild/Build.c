#include "OrynBuild.h"
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

static unsigned long long ComputeSourceBuildHash(const OrynProject* project, const char* source_file)
{
    unsigned long long hash = 1469598103934665603ULL;
    hash = OrynHashText(hash, ORYN_VERSION);
    hash = OrynHashText(hash, "\nsource:\n");
    (void)OrynHashFile(&hash, source_file);

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

static int CompileSourceFile(const OrynProject* project, const char* source_file, char* object_file, size_t object_file_size)
{
    char base_name[ORYN_MAX_PATH];
    char object_name[ORYN_MAX_PATH];
    OrynGetBaseName(base_name, sizeof(base_name), source_file);
    OrynReplaceExtension(object_name, sizeof(object_name), base_name, ".o");
    OrynJoinPath(object_file, object_file_size, project->object_dir, object_name);

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

    char command[ORYN_MAX_PATH * 6];
    snprintf(command, sizeof(command),
        "clang --target=x86_64-none-elf -ffreestanding -fno-stack-protector "
        "-fno-stack-check -fno-builtin -fno-pic -fno-pie -mno-red-zone -m64 "
        "-Wall -Wextra -I\"%s\" %s-I\"%s\" -I\"%s\" -I\"%s\" -c \"%s\" -o \"%s\"",
        project->include_dir,
        selection_include_argument,
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
    if (!OrynCollectCFiles(project->source_dir, &sources) || sources.count == 0)
    {
        OrynLogFail("No kernel source files were found.");
        return 0;
    }

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
