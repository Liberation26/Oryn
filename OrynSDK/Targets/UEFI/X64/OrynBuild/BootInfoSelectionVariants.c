#include "TargetBuildInternal.h"
int CollectVariantNumbers(const char* root, int* numbers, int max_numbers)
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

int LoadVariantByNumber(const char* root, int number, int* kernel_range,
    int* memory_map, int* framebuffer, int* rsdp, int* firmware_data)
{
    char folder_name[32];
    snprintf(folder_name, sizeof(folder_name), "%d", number);
    return ReadVariantSelection(root, folder_name, kernel_range, memory_map, framebuffer, rsdp, firmware_data);
}

void WriteSelectionNames(FILE* file, int kernel_range, int memory_map,
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

void PrintSelectionNames(int kernel_range, int memory_map, int framebuffer, int rsdp, int firmware_data)
{
    WriteSelectionNames(stdout, kernel_range, memory_map, framebuffer, rsdp, firmware_data);
}

void PrintVariantLine(int number, int selected, int kernel_range, int memory_map,
    int framebuffer, int rsdp, int firmware_data)
{
    unsigned long long mask = BootInfoSelectionMask(kernel_range, memory_map, framebuffer, rsdp, firmware_data);
    printf("%c Kernel/%d  BootInfoMask=0x%llX  ", selected ? '*' : ' ', number, mask);
    PrintSelectionNames(kernel_range, memory_map, framebuffer, rsdp, firmware_data);
    printf("\n");
}

int ValidateVariantExists(const OrynProject* project, int number)
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

int SelectedVariantNumber(const OrynProject* project)
{
    char selected_path[ORYN_MAX_PATH];
    OrynJoinPath(selected_path, sizeof(selected_path), project->kernel_variants_root, "Selected.txt");
    return ReadSelectedVariantNumber(selected_path);
}

int ShowVariantDetails(const OrynProject* project, int number)
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

int OrynBootInfoList(const OrynProject* project)
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

int OrynBootInfoShow(const OrynProject* project, int argument_count, char** arguments)
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

int OrynBootInfoSelect(const OrynProject* project, int argument_count, char** arguments)
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

void PrintCompareLine(const char* label, int left, int right)
{
    printf("  %-24s Kernel A: %-3s  Kernel B: %-3s  %s\n",
        label,
        YesNoText(left),
        YesNoText(right),
        left == right ? "same" : "different");
}

int OrynBootInfoCompare(const OrynProject* project, int argument_count, char** arguments)
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

int OrynBootInfoRun(const char* executable_path, const OrynProject* project,
    const char* project_file, int argument_count, char** arguments)
{
    if (OrynBootInfoSelect(project, argument_count, arguments) != 0)
    {
        return 1;
    }

    return OrynCommandRun(executable_path, project_file);
}

void WriteVariantTestReportHeader(FILE* file, const OrynProject* project)
{
    fprintf(file, "Oryn Kernel-5 BootInfo Variant Test Report\n");
    fprintf(file, "Version: %s\n", ORYN_VERSION);
    fprintf(file, "Project: %s\n", project->name);
    fprintf(file, "Variant root: %s\n\n", project->kernel_variants_root);
}

