#include "OrynBuild.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>


static int EndsWithTtf(const char* name)
{
    size_t length = strlen(name);
    if (length < 4U)
    {
        return 0;
    }

    const char* extension = name + length - 4U;
    return (extension[0] == '.' &&
        (extension[1] == 't' || extension[1] == 'T') &&
        (extension[2] == 't' || extension[2] == 'T') &&
        (extension[3] == 'f' || extension[3] == 'F'));
}

static int FindProjectTtfFont(const OrynProject* project, char* output, size_t output_size)
{
    char fonts_dir[ORYN_MAX_PATH];
    OrynJoinPath(fonts_dir, sizeof(fonts_dir), project->project_root, "System/Fonts");

    char preferred[ORYN_MAX_PATH];
    OrynJoinPath(preferred, sizeof(preferred), fonts_dir, "OrynSans.ttf");
    if (OrynFileExists(preferred))
    {
        snprintf(output, output_size, "%s", preferred);
        return 1;
    }

    DIR* dir = opendir(fonts_dir);
    if (dir == 0)
    {
        return 0;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != 0)
    {
        if (entry->d_name[0] == '.')
        {
            continue;
        }

        if (!EndsWithTtf(entry->d_name))
        {
            continue;
        }

        OrynJoinPath(output, output_size, fonts_dir, entry->d_name);
        closedir(dir);
        return 1;
    }

    closedir(dir);
    return 0;
}

static int SourceListContainsBaseName(const OrynStringList* sources, const char* base_name)
{
    for (int index = 0; index < sources->count; ++index)
    {
        char candidate[ORYN_MAX_PATH];
        OrynGetBaseName(candidate, sizeof(candidate), sources->items[index]);
        if (strcmp(candidate, base_name) == 0)
        {
            return 1;
        }
    }

    return 0;
}

static int CompileBootLoaderSource(const OrynProject* project, const char* source_file, char* object_file, size_t object_file_size)
{
    char loader_include[ORYN_MAX_PATH];
    char handoff_include[ORYN_MAX_PATH];
    char target_boot_include[ORYN_MAX_PATH];
    char base_name[ORYN_MAX_PATH];
    char object_name[ORYN_MAX_PATH];
    char loader_objects[ORYN_MAX_PATH];

    OrynJoinPath(loader_include, sizeof(loader_include), project->sdk_root, "Targets/UEFI/X64/Loader/Include");
    OrynJoinPath(handoff_include, sizeof(handoff_include), project->sdk_root, "Common/Handoff/Include");
    OrynJoinPath(target_boot_include, sizeof(target_boot_include), project->sdk_root, "Targets/UEFI/X64/Boot/Include");
    OrynJoinPath(loader_objects, sizeof(loader_objects), project->build_dir, "LoaderObjects");
    OrynGetBaseName(base_name, sizeof(base_name), source_file);
    OrynReplaceExtension(object_name, sizeof(object_name), base_name, ".obj");
    OrynJoinPath(object_file, object_file_size, loader_objects, object_name);

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
        "clang --target=x86_64-pc-windows-msvc -ffreestanding -fshort-wchar "
        "-mno-red-zone -fno-stack-protector -fno-builtin -Wno-unused-parameter "
        "%s-I\"%s\" -I\"%s\" -I\"%s\" -c \"%s\" -o \"%s\"",
        selection_include_argument,
        loader_include,
        handoff_include,
        target_boot_include,
        source_file,
        object_file);

    return OrynRunCommand(command);
}

static int BuildBootLoader(const OrynProject* project)
{
    OrynLogStep("Building UEFI loader.");
    if (project->selected_kernel_number > 0)
    {
        char message[ORYN_MAX_PATH + 128];
        snprintf(message, sizeof(message), "Using BootInfo variant Kernel/%d", project->selected_kernel_number);
        OrynLogOk(message);
    }

    char loader_objects_dir[ORYN_MAX_PATH];
    OrynJoinPath(loader_objects_dir, sizeof(loader_objects_dir), project->build_dir, "LoaderObjects");
    OrynMakeDirectoryRecursive(loader_objects_dir);

    char loader_source_dir[ORYN_MAX_PATH];
    OrynJoinPath(loader_source_dir, sizeof(loader_source_dir), project->sdk_root, "Targets/UEFI/X64/Loader/Source");

    OrynStringList sources;
    if (!OrynCollectCFiles(loader_source_dir, &sources) || sources.count == 0)
    {
        OrynLogFail("No UEFI loader source files were found.");
        return 0;
    }

    if (!SourceListContainsBaseName(&sources, "BootX64Entry.c"))
    {
        OrynLogFail("UEFI loader entry source BootX64Entry.c is missing.");
        return 0;
    }

    OrynLogOk("UEFI loader entry source: BootX64Entry.c");

    OrynStringList objects;
    objects.count = 0;
    for (int index = 0; index < sources.count; ++index)
    {
        if (!CompileBootLoaderSource(project, sources.items[index], objects.items[objects.count], ORYN_MAX_PATH))
        {
            OrynLogFail("UEFI loader compile failed.");
            return 0;
        }
        objects.count += 1;
    }

    char boot_dir[ORYN_MAX_PATH];
    OrynJoinPath(boot_dir, sizeof(boot_dir), project->esp_dir, "EFI/BOOT");
    OrynMakeDirectoryRecursive(boot_dir);

    char boot_efi[ORYN_MAX_PATH];
    OrynJoinPath(boot_efi, sizeof(boot_efi), boot_dir, "BOOTX64.EFI");

    char command[ORYN_MAX_PATH * 8];
    snprintf(command, sizeof(command),
        "lld-link /subsystem:efi_application /entry:efi_main /nodefaultlib /dll /out:\"%s\"",
        boot_efi);

    OrynLogOk("UEFI loader linker entry: /entry:efi_main");

    for (int index = 0; index < objects.count; ++index)
    {
        strncat(command, " \"", sizeof(command) - strlen(command) - 1);
        strncat(command, objects.items[index], sizeof(command) - strlen(command) - 1);
        strncat(command, "\"", sizeof(command) - strlen(command) - 1);
    }

    if (!OrynRunCommand(command))
    {
        OrynLogFail("UEFI loader link failed.");
        return 0;
    }

    if (!OrynFileExists(boot_efi))
    {
        OrynLogFail("UEFI loader output EFI/BOOT/BOOTX64.EFI was not produced.");
        return 0;
    }

    OrynLogOk("UEFI loader entry point exists and produced EFI/BOOT/BOOTX64.EFI");
    OrynLogOk("Built EFI/BOOT/BOOTX64.EFI");
    return 1;
}

int OrynBuildImage(const OrynProject* project)
{
    OrynLogStep("Creating FAT32 ESP disk image.");
    char message[ORYN_MAX_PATH + 128];
    OrynMakeDirectoryRecursive(project->esp_dir);

    if (!BuildBootLoader(project))
    {
        return 0;
    }

    char boot_efi[ORYN_MAX_PATH];
    OrynJoinPath(boot_efi, sizeof(boot_efi), project->esp_dir, "EFI/BOOT/BOOTX64.EFI");

    char kernel_source[ORYN_MAX_PATH];
    OrynJoinPath(kernel_source, sizeof(kernel_source), project->build_dir, "Kernel.elf");

    char system_dir[ORYN_MAX_PATH];
    OrynJoinPath(system_dir, sizeof(system_dir), project->esp_dir, "System");

    char kernel_dir[ORYN_MAX_PATH];
    OrynJoinPath(kernel_dir, sizeof(kernel_dir), system_dir, project->name);
    OrynMakeDirectoryRecursive(kernel_dir);

    char kernel_target[ORYN_MAX_PATH];
    OrynJoinPath(kernel_target, sizeof(kernel_target), kernel_dir, "Kernel.elf");

    if (!OrynCopyFile(kernel_source, kernel_target))
    {
        OrynLogFail("Could not copy Kernel.elf into ESP.");
        return 0;
    }

    char font_source[ORYN_MAX_PATH];
    int has_font = FindProjectTtfFont(project, font_source, sizeof(font_source));
    if (has_font)
    {
        char fonts_target_dir[ORYN_MAX_PATH];
        OrynJoinPath(fonts_target_dir, sizeof(fonts_target_dir), system_dir, "Fonts");
        OrynMakeDirectoryRecursive(fonts_target_dir);

        char font_target[ORYN_MAX_PATH];
        OrynJoinPath(font_target, sizeof(font_target), fonts_target_dir, "OrynSans.ttf");
        if (!OrynCopyFile(font_source, font_target))
        {
            OrynLogFail("Could not copy TTF font into staged ESP.");
            return 0;
        }

        snprintf(message, sizeof(message), "Copied TTF font to staged ESP: System/Fonts/OrynSans.ttf");
        OrynLogOk(message);
    }
    else
    {
        OrynLogWarn("No project TTF font found in System/Fonts. FAT32 image will contain an empty System/Fonts directory.");
    }

    char image_name[256];
    snprintf(image_name, sizeof(image_name), "%s.img", project->name);

    char disk_image[ORYN_MAX_PATH];
    OrynJoinPath(disk_image, sizeof(disk_image), project->output_dir, image_name);
    remove(disk_image);

    if (!OrynCreateFat32EspImage(boot_efi, kernel_source, has_font ? font_source : 0, disk_image, project->name))
    {
        OrynLogFail("Could not create FAT32 partition disk image.");
        return 0;
    }

    snprintf(message, sizeof(message), "Copied pure ELF kernel to staged ESP: System/%s/Kernel.elf", project->name);
    OrynLogOk(message);

    snprintf(message, sizeof(message), "Created Output/%s with an MBR FAT32 EFI System Partition.", image_name);
    OrynLogOk(message);
    return 1;
}
