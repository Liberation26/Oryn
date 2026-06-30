#include "TargetBuildInternal.h"
#include "OrynBuild.h"
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void Trim(char* text)
{
    char* start = text;
    while (*start != 0 && isspace((unsigned char)*start))
    {
        ++start;
    }

    if (start != text)
    {
        memmove(text, start, strlen(start) + 1);
    }

    size_t length = strlen(text);
    while (length > 0 && isspace((unsigned char)text[length - 1]))
    {
        text[length - 1] = 0;
        --length;
    }
}

int IsNumberText(const char* text)
{
    if (text == 0 || text[0] == 0)
    {
        return 0;
    }

    for (int index = 0; text[index] != 0; ++index)
    {
        if (!isdigit((unsigned char)text[index]))
        {
            return 0;
        }
    }

    return 1;
}

int ReadSelectedVariantNumber(const char* selected_path)
{
    FILE* file = fopen(selected_path, "rb");
    if (file == 0)
    {
        return 0;
    }

    char line[64];
    if (fgets(line, sizeof(line), file) == 0)
    {
        fclose(file);
        return 0;
    }

    fclose(file);
    Trim(line);
    if (!IsNumberText(line))
    {
        return 0;
    }

    return atoi(line);
}

void OrynResolveBootInfoSelection(OrynProject* project)
{
    project->selected_kernel_number = 0;
    project->selected_kernel_dir[0] = 0;
    project->selected_kernel_include_dir[0] = 0;

    char selected_path[ORYN_MAX_PATH];
    OrynJoinPath(selected_path, sizeof(selected_path), project->kernel_variants_root, "Selected.txt");

    int selected = ReadSelectedVariantNumber(selected_path);
    if (selected <= 0)
    {
        return;
    }

    char folder_name[32];
    snprintf(folder_name, sizeof(folder_name), "%d", selected);
    OrynJoinPath(project->selected_kernel_dir, sizeof(project->selected_kernel_dir),
        project->kernel_variants_root, folder_name);
    OrynJoinPath(project->selected_kernel_include_dir, sizeof(project->selected_kernel_include_dir),
        project->selected_kernel_dir, "Include");

    char header_path[ORYN_MAX_PATH];
    OrynJoinPath(header_path, sizeof(header_path), project->selected_kernel_include_dir,
        "OrynBootInfoSelection.h");

    if (!OrynFileExists(header_path))
    {
        OrynLogWarn("Selected BootInfo variant header was not found. Using SDK defaults.");
        project->selected_kernel_number = 0;
        project->selected_kernel_dir[0] = 0;
        project->selected_kernel_include_dir[0] = 0;
        return;
    }

    project->selected_kernel_number = selected;
}

int NextVariantNumber(const char* root)
{
    DIR* directory = opendir(root);
    if (directory == 0)
    {
        return 1;
    }

    int highest = 0;
    struct dirent* entry;
    while ((entry = readdir(directory)) != 0)
    {
        if (IsNumberText(entry->d_name))
        {
            int value = atoi(entry->d_name);
            if (value > highest)
            {
                highest = value;
            }
        }
    }

    closedir(directory);
    return highest + 1;
}

unsigned long long BootInfoSelectionMask(
    int kernel_range,
    int memory_map,
    int framebuffer,
    int rsdp,
    int firmware_data)
{
    unsigned long long mask = 0ULL;
    mask |= kernel_range ? 0x0000000000000008ULL : 0ULL;
    mask |= memory_map ? 0x0000000000000001ULL : 0ULL;
    mask |= framebuffer ? 0x0000000000000002ULL : 0ULL;
    mask |= rsdp ? 0x0000000000000004ULL : 0ULL;
    if (firmware_data)
    {
        mask |= 0x0000000000000020ULL;
        mask |= 0x0000000000000040ULL;
        mask |= 0x0000000000000080ULL;
        mask |= 0x0000000000000100ULL;
    }
    return mask;
}

int ReadHeaderDefineInt(const char* header_path, const char* define_name, int* value)
{
    FILE* file = fopen(header_path, "rb");
    if (file == 0)
    {
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), file) != 0)
    {
        char found_name[160];
        int found_value = 0;
        if (sscanf(line, "#define %159s %d", found_name, &found_value) == 2 &&
            strcmp(found_name, define_name) == 0)
        {
            fclose(file);
            *value = found_value;
            return 1;
        }
    }

    fclose(file);
    return 0;
}

int ReadVariantSelection(const char* root, const char* folder_name,
    int* kernel_range, int* memory_map, int* framebuffer, int* rsdp, int* firmware_data)
{
    char variant_dir[ORYN_MAX_PATH];
    char include_dir[ORYN_MAX_PATH];
    char header_path[ORYN_MAX_PATH];

    OrynJoinPath(variant_dir, sizeof(variant_dir), root, folder_name);
    OrynJoinPath(include_dir, sizeof(include_dir), variant_dir, "Include");
    OrynJoinPath(header_path, sizeof(header_path), include_dir, "OrynBootInfoSelection.h");

    if (!OrynFileExists(header_path))
    {
        return 0;
    }

    return ReadHeaderDefineInt(header_path, "ORYN_BOOTINFO_WANT_KERNEL_RANGE", kernel_range) &&
        ReadHeaderDefineInt(header_path, "ORYN_BOOTINFO_WANT_MEMORY_MAP", memory_map) &&
        ReadHeaderDefineInt(header_path, "ORYN_BOOTINFO_WANT_FRAMEBUFFER", framebuffer) &&
        ReadHeaderDefineInt(header_path, "ORYN_BOOTINFO_WANT_RSDP", rsdp) &&
        ReadHeaderDefineInt(header_path, "ORYN_BOOTINFO_WANT_FIRMWARE_DATA", firmware_data);
}

int FindExistingVariantNumber(const char* root, int kernel_range, int memory_map,
    int framebuffer, int rsdp, int firmware_data)
{
    DIR* directory = opendir(root);
    if (directory == 0)
    {
        return 0;
    }

    int best_match = 0;
    struct dirent* entry;
    while ((entry = readdir(directory)) != 0)
    {
        if (!IsNumberText(entry->d_name))
        {
            continue;
        }

        int existing_kernel_range = 0;
        int existing_memory_map = 0;
        int existing_framebuffer = 0;
        int existing_rsdp = 0;
        int existing_firmware_data = 0;

        if (!ReadVariantSelection(root, entry->d_name, &existing_kernel_range,
                &existing_memory_map, &existing_framebuffer, &existing_rsdp, &existing_firmware_data))
        {
            continue;
        }

        if (existing_kernel_range == kernel_range &&
            existing_memory_map == memory_map &&
            existing_framebuffer == framebuffer &&
            existing_rsdp == rsdp &&
            existing_firmware_data == firmware_data)
        {
            int value = atoi(entry->d_name);
            if (best_match == 0 || value < best_match)
            {
                best_match = value;
            }
        }
    }

    closedir(directory);
    return best_match;
}

int AskYesNo(const char* question, int default_yes)
{
    char line[64];
    for (;;)
    {
        printf("%s [%c/%c]: ", question, default_yes ? 'Y' : 'y', default_yes ? 'n' : 'N');
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == 0)
        {
            return default_yes;
        }

        Trim(line);
        if (line[0] == 0)
        {
            return default_yes;
        }

        if (line[0] == 'y' || line[0] == 'Y')
        {
            return 1;
        }

        if (line[0] == 'n' || line[0] == 'N')
        {
            return 0;
        }

        OrynLogWarn("Please answer y or n.");
    }
}

int WriteSelectedNumber(const char* root, int number)
{
    char selected_path[ORYN_MAX_PATH];
    OrynJoinPath(selected_path, sizeof(selected_path), root, "Selected.txt");

    FILE* file = fopen(selected_path, "wb");
    if (file == 0)
    {
        return 0;
    }

    fprintf(file, "%d\n", number);
    fclose(file);
    return 1;
}

int WriteSelectionHeader(const char* include_dir, int number, int kernel_range,
    int memory_map, int framebuffer, int rsdp, int firmware_data)
{
    char header_path[ORYN_MAX_PATH];
    OrynJoinPath(header_path, sizeof(header_path), include_dir, "OrynBootInfoSelection.h");

    FILE* file = fopen(header_path, "wb");
    if (file == 0)
    {
        return 0;
    }

    unsigned long long mask = BootInfoSelectionMask(kernel_range, memory_map, framebuffer, rsdp, firmware_data);

    fprintf(file, "#ifndef ORYN_BOOT_INFO_SELECTION_H\n");
    fprintf(file, "#define ORYN_BOOT_INFO_SELECTION_H\n\n");
    fprintf(file, "#define ORYN_BOOTINFO_SELECTION_NUMBER %d\n", number);
    fprintf(file, "#define ORYN_BOOTINFO_SELECTION_NAME \"Kernel-5/Kernel/%d\"\n", number);
    fprintf(file, "#define ORYN_BOOTINFO_SELECTION_MASK 0x%llX\n\n", mask);
    fprintf(file, "#define ORYN_BOOTINFO_WANT_KERNEL_RANGE %d\n", kernel_range);
    fprintf(file, "#define ORYN_BOOTINFO_WANT_MEMORY_MAP %d\n", memory_map);
    fprintf(file, "#define ORYN_BOOTINFO_WANT_FRAMEBUFFER %d\n", framebuffer);
    fprintf(file, "#define ORYN_BOOTINFO_WANT_RSDP %d\n", rsdp);
    fprintf(file, "#define ORYN_BOOTINFO_WANT_FIRMWARE_DATA %d\n", firmware_data);
    fprintf(file, "#define ORYN_BOOTINFO_WANT_PLATFORM_TABLES %d\n", firmware_data);
    fprintf(file, "#define ORYN_BOOTINFO_WANT_NVRAM %d\n", firmware_data);
    fprintf(file, "#define ORYN_BOOTINFO_WANT_RUNTIME_SERVICES %d\n\n", firmware_data);
    fprintf(file, "#endif\n");
    fclose(file);
    return 1;
}

int WriteVariantNotes(const char* variant_dir, int number, int kernel_range,
    int memory_map, int framebuffer, int rsdp, int firmware_data)
{
    char notes_path[ORYN_MAX_PATH];
    OrynJoinPath(notes_path, sizeof(notes_path), variant_dir, "BootInfoSelection.txt");

    FILE* file = fopen(notes_path, "wb");
    if (file == 0)
    {
        return 0;
    }

    unsigned long long mask = BootInfoSelectionMask(kernel_range, memory_map, framebuffer, rsdp, firmware_data);

    fprintf(file, "Oryn Kernel-5 BootInfo selection %d\n", number);
    fprintf(file, "BootInfoSelectionKey=0x%llX\n", mask);
    fprintf(file, "\n");
    fprintf(file, "Selected UEFI BootInfo items:\n");
    fprintf(file, "  Kernel physical range: %s\n", kernel_range ? "yes" : "no");
    fprintf(file, "  UEFI memory map: %s\n", memory_map ? "yes" : "no");
    fprintf(file, "  Linear framebuffer: %s\n", framebuffer ? "yes" : "no");
    fprintf(file, "  ACPI RSDP pointer: %s\n", rsdp ? "yes" : "no");
    fprintf(file, "  UEFI firmware data: %s\n", firmware_data ? "yes" : "no");
    fprintf(file, "  UEFI configuration tables / platform pointers: %s\n", firmware_data ? "yes" : "no");
    fprintf(file, "  UEFI NVRAM snapshot: %s\n", firmware_data ? "yes" : "no");
    fprintf(file, "  UEFI RuntimeServices pointers: %s\n", firmware_data ? "yes" : "no");
    fprintf(file, "\n");
    fprintf(file, "Generated files are used by both the UEFI loader and the kernel build.\n");
    fclose(file);
    return 1;
}

const char* YesNoText(int value)
{
    return value ? "yes" : "no";
}

int ParseVariantNumberText(const char* text, int* number)
{
    if (!IsNumberText(text))
    {
        return 0;
    }

    int value = atoi(text);
    if (value <= 0)
    {
        return 0;
    }

    *number = value;
    return 1;
}

int CompareNumbers(const void* left, const void* right)
{
    int a = *(const int*)left;
    int b = *(const int*)right;
    if (a < b)
    {
        return -1;
    }
    if (a > b)
    {
        return 1;
    }
    return 0;
}

