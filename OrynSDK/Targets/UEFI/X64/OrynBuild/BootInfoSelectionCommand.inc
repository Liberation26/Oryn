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
    printf("  1. Kernel physical range: physical base and loaded size of the OS-named kernel ELF.\n");
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
