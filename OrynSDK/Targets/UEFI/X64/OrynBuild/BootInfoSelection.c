#include "OrynBuild.h"
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void Trim(char* text)
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

static int IsNumberText(const char* text)
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

static int ReadSelectedVariantNumber(const char* selected_path)
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

static int NextVariantNumber(const char* root)
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

static unsigned long long BootInfoSelectionMask(
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

static int ReadHeaderDefineInt(const char* header_path, const char* define_name, int* value)
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

static int ReadVariantSelection(const char* root, const char* folder_name,
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

static int FindExistingVariantNumber(const char* root, int kernel_range, int memory_map,
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

static int AskYesNo(const char* question, int default_yes)
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

static int WriteSelectedNumber(const char* root, int number)
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

static int WriteSelectionHeader(const char* include_dir, int number, int kernel_range,
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

static int WriteVariantNotes(const char* variant_dir, int number, int kernel_range,
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

static const char* YesNoText(int value)
{
    return value ? "yes" : "no";
}

static int ParseVariantNumberText(const char* text, int* number)
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

static int CompareNumbers(const void* left, const void* right)
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

static int CollectVariantNumbers(const char* root, int* numbers, int max_numbers)
{
    DIR* directory = opendir(root);
    if (directory == 0)
    {
        return 0;
    }

    int count = 0;
    struct dirent* entry;
    while ((entry = readdir(directory)) != 0)
    {
        int number = 0;
        if (!ParseVariantNumberText(entry->d_name, &number))
        {
            continue;
        }

        if (count < max_numbers)
        {
            numbers[count] = number;
            ++count;
        }
    }

    closedir(directory);
    qsort(numbers, (size_t)count, sizeof(int), CompareNumbers);
    return count;
}

static int LoadVariantByNumber(const char* root, int number, int* kernel_range,
    int* memory_map, int* framebuffer, int* rsdp, int* firmware_data)
{
    char folder_name[32];
    snprintf(folder_name, sizeof(folder_name), "%d", number);
    return ReadVariantSelection(root, folder_name, kernel_range, memory_map, framebuffer, rsdp, firmware_data);
}

static void WriteSelectionNames(FILE* file, int kernel_range, int memory_map,
    int framebuffer, int rsdp, int firmware_data)
{
    int wrote_any = 0;
    if (kernel_range)
    {
        fprintf(file, "KernelRange");
        wrote_any = 1;
    }
    if (memory_map)
    {
        fprintf(file, "%sMemoryMap", wrote_any ? " " : "");
        wrote_any = 1;
    }
    if (framebuffer)
    {
        fprintf(file, "%sFramebuffer", wrote_any ? " " : "");
        wrote_any = 1;
    }
    if (rsdp)
    {
        fprintf(file, "%sRSDP", wrote_any ? " " : "");
        wrote_any = 1;
    }
    if (firmware_data)
    {
        fprintf(file, "%sFirmwareData PlatformTables Nvram RuntimeServices", wrote_any ? " " : "");
        wrote_any = 1;
    }
    if (!wrote_any)
    {
        fprintf(file, "None");
    }
}

static void PrintSelectionNames(int kernel_range, int memory_map, int framebuffer, int rsdp, int firmware_data)
{
    WriteSelectionNames(stdout, kernel_range, memory_map, framebuffer, rsdp, firmware_data);
}

static void PrintVariantLine(int number, int selected, int kernel_range, int memory_map,
    int framebuffer, int rsdp, int firmware_data)
{
    unsigned long long mask = BootInfoSelectionMask(kernel_range, memory_map, framebuffer, rsdp, firmware_data);
    printf("%c Kernel/%d  BootInfoMask=0x%llX  ", selected ? '*' : ' ', number, mask);
    PrintSelectionNames(kernel_range, memory_map, framebuffer, rsdp, firmware_data);
    printf("\n");
}

static int ValidateVariantExists(const OrynProject* project, int number)
{
    int kernel_range = 0;
    int memory_map = 0;
    int framebuffer = 0;
    int rsdp = 0;
    int firmware_data = 0;

    if (LoadVariantByNumber(project->kernel_variants_root, number, &kernel_range,
            &memory_map, &framebuffer, &rsdp, &firmware_data))
    {
        return 1;
    }

    char message[128];
    snprintf(message, sizeof(message), "Kernel/%d was not found or has no valid BootInfo selection header.", number);
    OrynLogFail(message);
    return 0;
}

static int SelectedVariantNumber(const OrynProject* project)
{
    char selected_path[ORYN_MAX_PATH];
    OrynJoinPath(selected_path, sizeof(selected_path), project->kernel_variants_root, "Selected.txt");
    return ReadSelectedVariantNumber(selected_path);
}

static int ShowVariantDetails(const OrynProject* project, int number)
{
    int kernel_range = 0;
    int memory_map = 0;
    int framebuffer = 0;
    int rsdp = 0;
    int firmware_data = 0;

    if (!LoadVariantByNumber(project->kernel_variants_root, number, &kernel_range,
            &memory_map, &framebuffer, &rsdp, &firmware_data))
    {
        char message[128];
        snprintf(message, sizeof(message), "Kernel/%d could not be read.", number);
        OrynLogFail(message);
        return 1;
    }

    int selected = SelectedVariantNumber(project) == number;
    unsigned long long mask = BootInfoSelectionMask(kernel_range, memory_map, framebuffer, rsdp, firmware_data);

    char folder_name[32];
    char variant_dir[ORYN_MAX_PATH];
    char header_path[ORYN_MAX_PATH];
    char notes_path[ORYN_MAX_PATH];
    snprintf(folder_name, sizeof(folder_name), "%d", number);
    OrynJoinPath(variant_dir, sizeof(variant_dir), project->kernel_variants_root, folder_name);
    OrynJoinPath(notes_path, sizeof(notes_path), variant_dir, "BootInfoSelection.txt");
    OrynJoinPath(header_path, sizeof(header_path), variant_dir, "Include/OrynBootInfoSelection.h");

    printf("Kernel-5 BootInfo variant\n\n");
    printf("Variant: Kernel/%d\n", number);
    printf("Selected: %s\n", selected ? "yes" : "no");
    printf("BootInfoMask: 0x%llX\n", mask);
    printf("Path: %s\n", variant_dir);
    printf("Header: %s\n", header_path);
    printf("Notes: %s\n\n", notes_path);

    printf("Items:\n");
    printf("  Kernel physical range: %s\n", YesNoText(kernel_range));
    printf("  UEFI memory map: %s\n", YesNoText(memory_map));
    printf("  Linear framebuffer: %s\n", YesNoText(framebuffer));
    printf("  ACPI RSDP pointer: %s\n", YesNoText(rsdp));
    printf("  UEFI firmware data: %s\n", YesNoText(firmware_data));
    printf("  UEFI configuration tables / platform pointers: %s\n", YesNoText(firmware_data));
    printf("  UEFI NVRAM snapshot: %s\n", YesNoText(firmware_data));
    printf("  UEFI RuntimeServices pointers: %s\n", YesNoText(firmware_data));
    return 0;
}

static int OrynBootInfoList(const OrynProject* project)
{
    int numbers[ORYN_MAX_ITEMS];
    int count = CollectVariantNumbers(project->kernel_variants_root, numbers, ORYN_MAX_ITEMS);
    int selected = SelectedVariantNumber(project);

    printf("Kernel-5 BootInfo variants\n\n");
    if (count <= 0)
    {
        printf("No numbered variants were found under:\n  %s\n", project->kernel_variants_root);
        return 1;
    }

    for (int index = 0; index < count; ++index)
    {
        int kernel_range = 0;
        int memory_map = 0;
        int framebuffer = 0;
        int rsdp = 0;
        int firmware_data = 0;
        if (!LoadVariantByNumber(project->kernel_variants_root, numbers[index], &kernel_range,
                &memory_map, &framebuffer, &rsdp, &firmware_data))
        {
            printf("  Kernel/%d  [invalid or missing OrynBootInfoSelection.h]\n", numbers[index]);
            continue;
        }

        PrintVariantLine(numbers[index], selected == numbers[index], kernel_range,
            memory_map, framebuffer, rsdp, firmware_data);
    }

    printf("\nSelected: ");
    if (selected > 0)
    {
        printf("Kernel/%d\n", selected);
    }
    else
    {
        printf("none\n");
    }

    return 0;
}

static int OrynBootInfoShow(const OrynProject* project, int argument_count, char** arguments)
{
    int number = 0;
    if (argument_count == 0)
    {
        number = SelectedVariantNumber(project);
        if (number <= 0)
        {
            OrynLogFail("No BootInfo variant is currently selected.");
            return 1;
        }
    }
    else if (!ParseVariantNumberText(arguments[0], &number))
    {
        OrynLogFail("bootinfo show expects a numeric variant number.");
        return 1;
    }

    return ShowVariantDetails(project, number);
}

static int OrynBootInfoSelect(const OrynProject* project, int argument_count, char** arguments)
{
    if (argument_count < 1)
    {
        OrynLogFail("bootinfo select expects a variant number.");
        return 1;
    }

    int number = 0;
    if (!ParseVariantNumberText(arguments[0], &number))
    {
        OrynLogFail("bootinfo select expects a numeric variant number.");
        return 1;
    }

    if (!ValidateVariantExists(project, number))
    {
        return 1;
    }

    if (!WriteSelectedNumber(project->kernel_variants_root, number))
    {
        OrynLogFail("Could not update Kernel/Selected.txt.");
        return 1;
    }

    char message[128];
    snprintf(message, sizeof(message), "Selected Kernel/%d.", number);
    OrynLogOk(message);
    return 0;
}

static void PrintCompareLine(const char* label, int left, int right)
{
    printf("  %-24s Kernel A: %-3s  Kernel B: %-3s  %s\n",
        label,
        YesNoText(left),
        YesNoText(right),
        left == right ? "same" : "different");
}

static int OrynBootInfoCompare(const OrynProject* project, int argument_count, char** arguments)
{
    if (argument_count < 2)
    {
        OrynLogFail("bootinfo compare expects two variant numbers.");
        return 1;
    }

    int left_number = 0;
    int right_number = 0;
    if (!ParseVariantNumberText(arguments[0], &left_number) ||
        !ParseVariantNumberText(arguments[1], &right_number))
    {
        OrynLogFail("bootinfo compare expects numeric variant numbers.");
        return 1;
    }

    int left_kernel_range = 0;
    int left_memory_map = 0;
    int left_framebuffer = 0;
    int left_rsdp = 0;
    int left_firmware_data = 0;
    int right_kernel_range = 0;
    int right_memory_map = 0;
    int right_framebuffer = 0;
    int right_rsdp = 0;
    int right_firmware_data = 0;

    if (!LoadVariantByNumber(project->kernel_variants_root, left_number, &left_kernel_range,
            &left_memory_map, &left_framebuffer, &left_rsdp, &left_firmware_data) ||
        !LoadVariantByNumber(project->kernel_variants_root, right_number, &right_kernel_range,
            &right_memory_map, &right_framebuffer, &right_rsdp, &right_firmware_data))
    {
        OrynLogFail("One or both BootInfo variants could not be read.");
        return 1;
    }

    unsigned long long left_mask = BootInfoSelectionMask(left_kernel_range, left_memory_map,
        left_framebuffer, left_rsdp, left_firmware_data);
    unsigned long long right_mask = BootInfoSelectionMask(right_kernel_range, right_memory_map,
        right_framebuffer, right_rsdp, right_firmware_data);

    printf("Kernel-5 BootInfo variant comparison\n\n");
    printf("Kernel A: Kernel/%d  BootInfoMask=0x%llX\n", left_number, left_mask);
    printf("Kernel B: Kernel/%d  BootInfoMask=0x%llX\n\n", right_number, right_mask);
    PrintCompareLine("Kernel physical range", left_kernel_range, right_kernel_range);
    PrintCompareLine("UEFI memory map", left_memory_map, right_memory_map);
    PrintCompareLine("Linear framebuffer", left_framebuffer, right_framebuffer);
    PrintCompareLine("ACPI RSDP pointer", left_rsdp, right_rsdp);
    PrintCompareLine("UEFI firmware data", left_firmware_data, right_firmware_data);

    if (left_mask == right_mask)
    {
        OrynLogOk("The two variants are equivalent.");
    }
    else
    {
        OrynLogInfo("The two variants are different.");
    }

    return 0;
}

static int OrynBootInfoRun(const char* executable_path, const OrynProject* project,
    const char* project_file, int argument_count, char** arguments)
{
    if (OrynBootInfoSelect(project, argument_count, arguments) != 0)
    {
        return 1;
    }

    return OrynCommandRun(executable_path, project_file);
}

static void WriteVariantTestReportHeader(FILE* file, const OrynProject* project)
{
    fprintf(file, "Oryn Kernel-5 BootInfo Variant Test Report\n");
    fprintf(file, "Version: %s\n", ORYN_VERSION);
    fprintf(file, "Project: %s\n", project->name);
    fprintf(file, "Variant root: %s\n\n", project->kernel_variants_root);
}

static int OrynBootInfoTestAll(const char* executable_path, const OrynProject* project,
    const char* project_file)
{
    int numbers[ORYN_MAX_ITEMS];
    int count = CollectVariantNumbers(project->kernel_variants_root, numbers, ORYN_MAX_ITEMS);
    if (count <= 0)
    {
        OrynLogFail("No numbered BootInfo variants were found to test.");
        return 1;
    }

    if (!OrynMakeDirectoryRecursive(project->output_dir))
    {
        OrynLogFail("Could not create project output directory for the test report.");
        return 1;
    }

    char report_path[ORYN_MAX_PATH];
    OrynJoinPath(report_path, sizeof(report_path), project->output_dir, "BootInfoVariantReport.txt");
    FILE* report = fopen(report_path, "wb");
    if (report == 0)
    {
        OrynLogFail("Could not write BootInfoVariantReport.txt.");
        return 1;
    }

    int original_selected = SelectedVariantNumber(project);
    int failures = 0;
    WriteVariantTestReportHeader(report, project);

    for (int index = 0; index < count; ++index)
    {
        int number = numbers[index];
        int kernel_range = 0;
        int memory_map = 0;
        int framebuffer = 0;
        int rsdp = 0;
        int firmware_data = 0;
        if (!LoadVariantByNumber(project->kernel_variants_root, number, &kernel_range,
                &memory_map, &framebuffer, &rsdp, &firmware_data))
        {
            fprintf(report, "Kernel/%d INVALID\n", number);
            ++failures;
            continue;
        }

        char message[128];
        snprintf(message, sizeof(message), "Testing Kernel/%d BootInfo variant.", number);
        OrynLogStep(message);

        if (!WriteSelectedNumber(project->kernel_variants_root, number))
        {
            fprintf(report, "Kernel/%d FAIL Could not select variant\n", number);
            ++failures;
            continue;
        }

        int result = OrynCommandRun(executable_path, project_file);
        fprintf(report, "Kernel/%d %s BootInfoMask=0x%llX ", number,
            result == 0 ? "PASS" : "FAIL",
            BootInfoSelectionMask(kernel_range, memory_map, framebuffer, rsdp, firmware_data));
        WriteSelectionNames(report, kernel_range, memory_map, framebuffer, rsdp, firmware_data);
        fprintf(report, "\n");

        if (result != 0)
        {
            ++failures;
        }
    }

    if (original_selected > 0)
    {
        WriteSelectedNumber(project->kernel_variants_root, original_selected);
        fprintf(report, "\nRestored selected variant: Kernel/%d\n", original_selected);
    }

    fclose(report);
    OrynLogKeyValue("Variant test report", report_path);

    if (failures == 0)
    {
        OrynLogOk("All BootInfo variants passed.");
        return 0;
    }

    char failure_message[128];
    snprintf(failure_message, sizeof(failure_message), "%d BootInfo variant test(s) failed.", failures);
    OrynLogFail(failure_message);
    return 1;
}

static int OrynBootInfoQuestionnaire(const char* executable_path, const char* project_file,
    const OrynProject* project)
{
    OrynLogStep("UEFI BootInfo questionnaire.");
    OrynLogKeyValue("Project", project->name);
    OrynLogKeyValue("Kernel variant root", project->kernel_variants_root);
    printf("\nChoose the BootInfo items that BOOTX64.EFI may pass to %s.\n", project->name);
    printf("A numbered kernel variant folder will be selected or created under:\n");
    printf("  %s/<number>\n\n", project->kernel_variants_root);

    printf("Available UEFI BootInfo items:\n");
    printf("  1. Kernel physical range: physical base and loaded size of Kernel.elf.\n");
    printf("  2. UEFI memory map: memory descriptors copied before ExitBootServices.\n");
    printf("  3. Linear framebuffer: framebuffer base, size, resolution, pitch, bytes-per-pixel, and pixel format.\n");
    printf("  4. ACPI RSDP pointer: ACPI root pointer discovered from UEFI configuration tables.\n");
    printf("  5. UEFI firmware/platform data: vendor, UEFI revision, boot time, ACPI/SMBIOS/FDT pointers, NVRAM snapshot, and RuntimeServices pointers.\n\n");

    int kernel_range = AskYesNo("Pass item 1, Kernel physical range?", 1);
    int memory_map = AskYesNo("Pass item 2, UEFI memory map?", 1);
    int framebuffer = AskYesNo("Pass item 3, Linear framebuffer?", 1);
    int rsdp = AskYesNo("Pass item 4, ACPI RSDP pointer?", 1);
    int firmware_data = AskYesNo("Pass item 5, UEFI firmware/platform data?", 1);

    if (!OrynMakeDirectoryRecursive(project->kernel_variants_root))
    {
        OrynLogFail("Could not create Kernel-5/Kernel variant root.");
        return 1;
    }

    int number = FindExistingVariantNumber(project->kernel_variants_root, kernel_range,
        memory_map, framebuffer, rsdp, firmware_data);
    int reused_existing = number > 0;

    if (!reused_existing)
    {
        number = NextVariantNumber(project->kernel_variants_root);
    }

    char folder_name[32];
    snprintf(folder_name, sizeof(folder_name), "%d", number);

    char variant_dir[ORYN_MAX_PATH];
    char include_dir[ORYN_MAX_PATH];
    OrynJoinPath(variant_dir, sizeof(variant_dir), project->kernel_variants_root, folder_name);
    OrynJoinPath(include_dir, sizeof(include_dir), variant_dir, "Include");

    if (reused_existing)
    {
        if (!WriteSelectedNumber(project->kernel_variants_root, number))
        {
            OrynLogFail("Could not select the existing BootInfo variant.");
            return 1;
        }

        OrynLogOk("Existing matching Kernel-5 BootInfo variant selected.");
        OrynLogKeyValue("Variant", variant_dir);
        OrynLogKeyValue("Selected", folder_name);
    }
    else
    {
        if (!OrynMakeDirectoryRecursive(include_dir) ||
            !WriteSelectionHeader(include_dir, number, kernel_range, memory_map, framebuffer, rsdp, firmware_data) ||
            !WriteVariantNotes(variant_dir, number, kernel_range, memory_map, framebuffer, rsdp, firmware_data) ||
            !WriteSelectedNumber(project->kernel_variants_root, number))
        {
            OrynLogFail("Could not write BootInfo variant files.");
            return 1;
        }

        OrynLogOk("Created numbered Kernel-5 BootInfo variant.");
        OrynLogKeyValue("Variant", variant_dir);
        OrynLogKeyValue("Selected", folder_name);
    }

    int run_now = AskYesNo("Build, image, and run this selected Kernel-5 variant now?", 1);
    if (!run_now)
    {
        OrynLogOk("BootInfo selection saved. Use ./Oryn.sh run to build and run it later.");
        return 0;
    }

    return OrynCommandRun(executable_path, project_file);
}

int OrynCommandBootInfo(const char* executable_path, const char* project_file,
    int argument_count, char** arguments)
{
    OrynProject project;
    if (!OrynLoadProject(executable_path, project_file, &project))
    {
        return 1;
    }

    if (argument_count <= 0 || strcmp(arguments[0], "new") == 0 ||
        strcmp(arguments[0], "questionnaire") == 0)
    {
        return OrynBootInfoQuestionnaire(executable_path, project_file, &project);
    }

    if (strcmp(arguments[0], "list") == 0)
    {
        return OrynBootInfoList(&project);
    }

    if (strcmp(arguments[0], "show") == 0)
    {
        return OrynBootInfoShow(&project, argument_count - 1, arguments + 1);
    }

    if (strcmp(arguments[0], "select") == 0)
    {
        return OrynBootInfoSelect(&project, argument_count - 1, arguments + 1);
    }

    if (strcmp(arguments[0], "compare") == 0)
    {
        return OrynBootInfoCompare(&project, argument_count - 1, arguments + 1);
    }

    if (strcmp(arguments[0], "run") == 0)
    {
        return OrynBootInfoRun(executable_path, &project, project_file,
            argument_count - 1, arguments + 1);
    }

    if (strcmp(arguments[0], "test-all") == 0)
    {
        return OrynBootInfoTestAll(executable_path, &project, project_file);
    }

    OrynLogFail("Unknown bootinfo subcommand.");
    printf("Usage:\n");
    printf("  oryn bootinfo <Project.oryn>\n");
    printf("  oryn bootinfo <Project.oryn> list\n");
    printf("  oryn bootinfo <Project.oryn> show [number]\n");
    printf("  oryn bootinfo <Project.oryn> select <number>\n");
    printf("  oryn bootinfo <Project.oryn> compare <left> <right>\n");
    printf("  oryn bootinfo <Project.oryn> run <number>\n");
    printf("  oryn bootinfo <Project.oryn> test-all\n");
    return 1;
}
