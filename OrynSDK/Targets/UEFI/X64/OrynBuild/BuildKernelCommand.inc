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

    char kernel_file_name[256];
    OrynMakeKernelElfFileName(kernel_file_name, sizeof(kernel_file_name), project->name);

    char kernel_elf[ORYN_MAX_PATH];
    OrynJoinPath(kernel_elf, sizeof(kernel_elf), project->build_dir, kernel_file_name);

    char command[ORYN_MAX_PATH * 8];
    snprintf(command, sizeof(command),
        "ld.lld -nostdlib -static -z max-page-size=0x1000 "
        "--defsym=ORYN_KERNEL_PHYSICAL_BASE=0x%llX "
        "--defsym=ORYN_KERNEL_VIRTUAL_BASE=0x%llX "
        "-T \"%s\" -o \"%s\"",
        project->kernel_physical_base,
        project->kernel_virtual_base,
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

    char layout_message[256];
    snprintf(layout_message, sizeof(layout_message),
        "Kernel physical load base: 0x%llX",
        project->kernel_physical_base);
    OrynLogOk(layout_message);

    snprintf(layout_message, sizeof(layout_message),
        "Kernel chosen virtual base: 0x%llX",
        project->kernel_virtual_base);
    OrynLogOk(layout_message);

    char link_message[ORYN_MAX_PATH + 64];
    snprintf(link_message, sizeof(link_message), "Linked Build/%s", kernel_file_name);
    OrynLogOk(link_message);
    return 1;
}
