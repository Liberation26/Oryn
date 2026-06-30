int OrynRunQemu(const OrynProject* project)
{
    char qemu_path[ORYN_MAX_PATH];
    if (!OrynFindWindowsQemu(qemu_path, sizeof(qemu_path)))
    {
        OrynLogFail("Windows QEMU could not be resolved from WSL.");
        OrynLogWarn("Expected default Windows QEMU path: /mnt/c/Program Files/qemu/qemu-system-x86_64.exe");
        return 0;
    }

    if (project->ovmf_path[0] == 0)
    {
        OrynLogFail("OVMF path could not be resolved.");
        return 0;
    }

    char ovmf_windows[ORYN_MAX_PATH];
    char ovmf_qemu[ORYN_MAX_PATH];
    char disk_image[ORYN_MAX_PATH];
    char stage_root[ORYN_MAX_PATH];
    char stage_disk_image[ORYN_MAX_PATH];
    char stage_debug_log[ORYN_MAX_PATH];
    char disk_windows[ORYN_MAX_PATH];
    char disk_qemu[ORYN_MAX_PATH];
    char stage_debug_windows[ORYN_MAX_PATH];
    char stage_debug_qemu[ORYN_MAX_PATH];

    BuildProjectImagePath(project, disk_image, sizeof(disk_image));
    if (!OrynFileExists(disk_image))
    {
        OrynLogFail("FAT32 disk image was not found. Run image first.");
        return 0;
    }

    if (!OrynResolveWindowsStageRoot(stage_root, sizeof(stage_root)))
    {
        OrynLogFail("Could not create a Windows-local QEMU staging folder.");
        OrynLogWarn("Set ORYN_QEMU_STAGE_DIR to a Windows-local folder if needed.");
        return 0;
    }

    char stage_project_name[512];
    BuildQemuStageProjectName(project, stage_project_name, sizeof(stage_project_name));
    OrynMakeStageFilePath(stage_disk_image, sizeof(stage_disk_image), stage_root, stage_project_name, ".img");
    OrynMakeStageFilePath(stage_debug_log, sizeof(stage_debug_log), stage_root, stage_project_name, "-Debug.log");
    remove(stage_disk_image);
    remove(stage_debug_log);

    if (!OrynCopyFile(disk_image, stage_disk_image))
    {
        OrynLogFail("Could not copy FAT32 disk image to Windows-local QEMU staging folder.");
        return 0;
    }

    if (!OrynConvertWslPathToWindows(project->ovmf_path, ovmf_windows, sizeof(ovmf_windows)) ||
        !OrynConvertWslPathToWindowsQemu(project->ovmf_path, ovmf_qemu, sizeof(ovmf_qemu)) ||
        !OrynConvertWslPathToWindows(stage_disk_image, disk_windows, sizeof(disk_windows)) ||
        !OrynConvertWslPathToWindowsQemu(stage_disk_image, disk_qemu, sizeof(disk_qemu)) ||
        !OrynConvertWslPathToWindows(stage_debug_log, stage_debug_windows, sizeof(stage_debug_windows)) ||
        !OrynConvertWslPathToWindowsQemu(stage_debug_log, stage_debug_qemu, sizeof(stage_debug_qemu)))
    {
        OrynLogFail("Could not convert one or more Windows-local staging paths for QEMU.");
        return 0;
    }

    OrynLogStep("Launching Windows QEMU from WSL.");
    OrynLogKeyValue("QEMU", qemu_path);
    OrynLogKeyValue("OVMF", ovmf_windows);
    OrynLogKeyValue("OVMF QEMU path", ovmf_qemu);
    OrynLogKeyValue("Firmware mode", "pflash");
    OrynLogKeyValue("FAT32 image source", disk_image);
    OrynLogKeyValue("FAT32 image staged", disk_windows);

    const char* display_mode = ResolveQemuDisplayMode(project);
    if (!IsSafeQemuDisplayMode(display_mode))
    {
        OrynLogFail("Project [Run] Display value contains unsupported characters.");
        OrynLogWarn("Use Display=none for headless or Display=sdl for a QEMU window.");
        return 0;
    }
    int interactive_display = IsInteractiveDisplayMode(display_mode);

    if (!TextEqualsIgnoreCaseCommand(project->run_vm, "QEMU"))
    {
        OrynLogFail("Only VM=QEMU is currently supported by Oryn WSL run.");
        return 0;
    }

    if (!IsSafeQemuArgumentValue(project->run_memory) ||
        !IsSafeQemuArgumentValue(project->run_cpu) ||
        !IsSafeQemuArgumentValue(project->run_smp))
    {
        OrynLogFail("VMSettings Memory, CPU, or SMP contains unsupported characters.");
        return 0;
    }

    if (!TextEqualsIgnoreCaseCommand(project->run_disk_format, "raw"))
    {
        OrynLogFail("Only DiskFormat=raw is currently supported by the built-in FAT32 image writer.");
        return 0;
    }

    if (!TextEqualsIgnoreCaseCommand(project->run_storage_interface, "ide"))
    {
        OrynLogFail("Only StorageInterface=ide is currently supported by the UEFI FAT32 image runner.");
        return 0;
    }

    int vm_pic = ProjectBoolEnabled(project->run_pic, 1);
    int vm_apic = ProjectBoolEnabled(project->run_apic, 1);
    int vm_apic2 = ProjectBoolEnabled(project->run_apic2, 1);
    int vm_hpet = ProjectBoolEnabled(project->run_hpet, 1);
    unsigned int vm_smp_count = ProjectCpuCount(project);
    const char* qemu_cpu_model = ResolveQemuCpuModel(project->run_cpu);
    if (QemuCpuWasTranslated(project->run_cpu, qemu_cpu_model))
    {
        OrynLogWarn("CPU=host/native requires KVM/HVF/WHX acceleration; using CPU=max for Windows QEMU from WSL.");
    }

    char smp_text[32];
    snprintf(smp_text, sizeof(smp_text), "%u", vm_smp_count);
    OrynLogKeyValue("Display", display_mode);
    OrynLogKeyValue("Output mode", interactive_display ?
        "graphical framebuffer screen window" :
        "headless terminal serial/debug output");
    if (interactive_display)
    {
        OrynLogInfo("Graphical screen mode: QEMU framebuffer output is in the QEMU window, not the terminal serial stream.");
        OrynLogInfo("Interactive display mode: QEMU will stay open for manual screen scrolling tests until you close the QEMU window.");
    }
    else
    {
        OrynLogInfo("Headless mode: terminal output is live serial/debug output, not the graphical framebuffer screen.");
    }
    OrynLogKeyValue("SMP CPUs", smp_text);
    OrynLogKeyValue("Memory", project->run_memory);
    OrynLogKeyValue("CPU requested", project->run_cpu);
    OrynLogKeyValue("CPU resolved", qemu_cpu_model);
    OrynLogKeyValue("PIC", OnOffText(vm_pic));
    OrynLogKeyValue("APIC", OnOffText(vm_apic));
    OrynLogKeyValue("APIC2/x2APIC", OnOffText(vm_apic2));
    OrynLogKeyValue("HPET", OnOffText(vm_hpet));
    OrynLogKeyValue("Disk format", project->run_disk_format);
    OrynLogKeyValue("Storage interface", project->run_storage_interface);

    char debug_log[ORYN_MAX_PATH];
    char boot_report[ORYN_MAX_PATH];
    OrynJoinPath(debug_log, sizeof(debug_log), project->output_dir, "Debug.log");
    OrynJoinPath(boot_report, sizeof(boot_report), project->output_dir, "BootReport.txt");
    remove(debug_log);
    remove(boot_report);

    const char* serial_argument = interactive_display ?
        "-serial null" :
        "-chardev stdio,id=orynserial,signal=off -serial chardev:orynserial";
    OrynLogKeyValue("Serial", interactive_display ?
        "null sink for faster interactive screen refresh" :
        "live WSL terminal stdio chardev");
    if (!interactive_display)
    {
        char timeout_text[64];
        snprintf(timeout_text, sizeof(timeout_text), "%u seconds", ResolveQemuHeadlessTimeoutSeconds());
        OrynLogKeyValue("Boot timeout", timeout_text);
    }
    OrynLogKeyValue("Debug log", debug_log);
    OrynLogKeyValue("Debug log staged", stage_debug_windows);
    OrynLogKeyValue("Boot report", boot_report);

    char firmware_argument[ORYN_MAX_PATH * 2];
    char drive_argument[ORYN_MAX_PATH * 2];
    char debug_argument[ORYN_MAX_PATH * 2];
    snprintf(firmware_argument, sizeof(firmware_argument), "if=pflash,format=raw,readonly=on,file=%s", ovmf_qemu);
    snprintf(drive_argument, sizeof(drive_argument), "format=raw,if=ide,file=%s", disk_qemu);
    snprintf(debug_argument, sizeof(debug_argument), "file:%s", stage_debug_qemu);

    char qemu_quoted[ORYN_MAX_PATH + 16];
    char firmware_quoted[(ORYN_MAX_PATH * 2) + 16];
    char drive_quoted[(ORYN_MAX_PATH * 2) + 16];
    char debug_quoted[(ORYN_MAX_PATH * 2) + 16];
    OrynShellQuote(qemu_quoted, sizeof(qemu_quoted), qemu_path);
    OrynShellQuote(firmware_quoted, sizeof(firmware_quoted), firmware_argument);
    OrynShellQuote(drive_quoted, sizeof(drive_quoted), drive_argument);
    OrynShellQuote(debug_quoted, sizeof(debug_quoted), debug_argument);

    char machine_argument[128];
    char cpu_argument[256];
    snprintf(machine_argument, sizeof(machine_argument), "q35,hpet=%s", vm_hpet ? "on" : "off");
    snprintf(cpu_argument, sizeof(cpu_argument), "%s,+apic,%sx2apic",
        qemu_cpu_model,
        vm_apic2 ? "+" : "-");

    char debug_exit_argument[96];
    if (interactive_display)
    {
        debug_exit_argument[0] = 0;
    }
    else
    {
        snprintf(debug_exit_argument, sizeof(debug_exit_argument),
            "-device isa-debug-exit,iobase=0xf4,iosize=0x04 ");
    }

    char qemu_command[ORYN_MAX_PATH * 8];
    snprintf(qemu_command, sizeof(qemu_command),
        "%s -machine %s -cpu %s -smp %u -m %s -drive %s -no-reboot -display %s "
        "-monitor none %s -debugcon %s -global isa-debugcon.iobase=0xe9 "
        "%s-boot order=c,menu=off -drive %s",
        qemu_quoted,
        machine_argument,
        cpu_argument,
        vm_smp_count,
        project->run_memory,
        firmware_quoted,
        display_mode,
        serial_argument,
        debug_quoted,
        debug_exit_argument,
        drive_quoted);

    char command[ORYN_MAX_PATH * 9];
    if (interactive_display)
    {
        snprintf(command, sizeof(command), "%s", qemu_command);
    }
    else
    {
        snprintf(command, sizeof(command), "timeout --foreground %us %s",
            ResolveQemuHeadlessTimeoutSeconds(), qemu_command);
    }

    int exit_code = -1;
    if (!interactive_display)
    {
        printf("\n======== LIVE TERMINAL SERIAL OUTPUT START: %s ========\n", project->name);
        printf("[INFO] This is serial/debug output. It is not graphical framebuffer screen output.\n");
        printf("[INFO] Waiting for UEFI loader marker: [BOOT] Stage 01\n");
        fflush(stdout);
    }
    else
    {
        printf("\n======== GRAPHICAL FRAMEBUFFER SCREEN TEST: %s ========\n", project->name);
        printf("[INFO] QEMU window output is the screen/framebuffer. Close the QEMU window to continue.\n");
        fflush(stdout);
    }

    int command_ok = RunQemuAndGetExitCode(command, &exit_code);

    if (!interactive_display)
    {
        printf("\n======== LIVE TERMINAL SERIAL OUTPUT END: %s ========\n", project->name);
        fflush(stdout);
    }

    if (OrynFileExists(stage_debug_log))
    {
        OrynCopyFile(stage_debug_log, debug_log);
    }

    size_t debug_text_capacity = 1024U * 1024U;
    char* debug_text = (char*)malloc(debug_text_capacity);
    if (debug_text == 0)
    {
        OrynLogFail("Could not allocate debug log read buffer.");
        return 0;
    }

    ReadFileText(debug_log, debug_text, debug_text_capacity);
    WriteBootReport(project, boot_report, qemu_path, ovmf_windows, ovmf_qemu, disk_windows,
        debug_log, qemu_cpu_model, command, debug_text, exit_code, command_ok);

    PrintFileIfPresent("QEMU debug output", debug_log);
    PrintFileIfPresent("Boot report", boot_report);

    int boot_pass = (TextContains(debug_text, "[KERNEL] PASS: Kernel entered successfully") ||
        TextContains(debug_text, "[KERNEL] Oryn Kernel-5 entered.")) &&
        TextContains(debug_text, "[KERNEL] PASS: BootInfo received") &&
        TextContains(debug_text, "[KERNEL] PASS: GDT installed.") &&
        TextContains(debug_text, "[KERNEL] GDT entries: 7") &&
        TextContains(debug_text, "[KERNEL] PASS: TSS loaded.") &&
        TextContains(debug_text, "[KERNEL] PASS: IDT installed.") &&
        TextContains(debug_text, "[KERNEL] PASS: Interrupt dispatcher initialized.") &&
        (ProjectBoolEnabled(project->run_pic, 1) ? TextContains(debug_text, "[KERNEL] PASS: PIC IRQ0 interrupt fired through IDT dispatch.") : TextContains(debug_text, "[KERNEL] INFO: PIC IRQ0 proof skipped by VMSettings.")) &&
        (ProjectBoolEnabled(project->run_apic2, 1) ? TextContains(debug_text, "[KERNEL] PASS: CPU APIC2/x2APIC feature present.") : 1) &&
        (ProjectBoolEnabled(project->run_pic, 1) ? TextContains(debug_text, "[KERNEL] PASS: PIC masked/disabled for APIC handoff.") : 1) &&
        (ProjectBoolEnabled(project->run_apic2, 1) ? TextContains(debug_text, "[KERNEL] PASS: APIC2/x2APIC mode enabled.") : 1) &&
        ((ProjectBoolEnabled(project->run_apic, 1) || ProjectBoolEnabled(project->run_apic2, 1)) ? TextContains(debug_text, "[KERNEL] PASS: APIC timer counter moved in masked probe.") : TextContains(debug_text, "[KERNEL] INFO: APIC proofs skipped by VMSettings.")) &&
        (ProjectBoolEnabled(project->run_hpet, 1) ? TextContains(debug_text, "[KERNEL] PASS: HPET counter advanced in probe.") : TextContains(debug_text, "[KERNEL] INFO: HPET proof skipped by VMSettings.")) &&
        ((ProjectBoolEnabled(project->run_apic, 1) || ProjectBoolEnabled(project->run_apic2, 1)) ? TextContains(debug_text, "[KERNEL] PASS: APIC timer interrupt fired through IDT dispatch.") : 1) &&
        (TextContains(debug_text, "[KERNEL] PASS: Interrupts work from PIC upward through APIC/APIC2.") ||
            TextContains(debug_text, "[KERNEL] PASS: VMSettings interrupt/timer profile applied.")) &&
        TextContains(debug_text, "[KERNEL] PASS: SysCalls use Get/Set/Event message packets.") &&
        TextContains(debug_text, "[KERNEL] PASS: Platform syscalls translate into Get/Set/Event packets.") &&
        TextContains(debug_text, "[KERNEL] PASS: SysCall header counts listed.") &&
        TextContains(debug_text, "[KERNEL] PASS: Unknown syscall debug logging path executed.") &&
        TextContains(debug_text, "[KERNEL] PASS: PCI ACPI MCFG table discovered.") &&
        TextContains(debug_text, "[KERNEL] PASS: PCIe ECAM descriptor captured.") &&
        TextContains(debug_text, "[KERNEL] PASS: PCI config mechanism #1 responded.") &&
        TextContains(debug_text, "[KERNEL] PASS: PCI bus/device/function scan completed.") &&
        TextContains(debug_text, "[KERNEL] PASS: PCI devices discovered.") &&
        TextContains(debug_text, "[KERNEL] PASS: PCI class-code decoding ready.") &&
        TextContains(debug_text, "[KERNEL] PASS: PCI device output uses English labels.") &&
        TextContains(debug_text, "[KERNEL] PASS: PCI Discovery complete.") &&
        ((ProjectCpuCount(project) > 1U && (ProjectBoolEnabled(project->run_apic, 1) || ProjectBoolEnabled(project->run_apic2, 1))) ?
            (TextContains(debug_text, "[KERNEL] PASS: SMP multi-core CPU topology discovered.") &&
             TextContains(debug_text, "[KERNEL] PASS: SMP application processors entered kernel AP loop.") &&
             TextContains(debug_text, "[KERNEL] PASS: Multi-Core processing initialized.")) :
            (TextContains(debug_text, "[KERNEL] INFO: SMP AP startup skipped by VMSettings.") ||
             TextContains(debug_text, "[KERNEL] INFO: SMP discovery skipped by VMSettings."))) &&
        TextContains(debug_text, "\033[32m[KERNEL] PASS") &&
        TextContains(debug_text, "\033[0m") &&
        !TextContains(debug_text, "[KERNEL] EXCEPTION:") &&
        TextContains(debug_text, "[KERNEL] PASS: Kernel screen presents dirty completed line only.") &&
        TextContains(debug_text, "[KERNEL] PASS: Kernel screen uses fast scroll path after visible area is full.") &&
        TextContains(debug_text, "[KERNEL] PASS: Kernel screen refresh is line/scroll optimized.") &&
        TextContains(debug_text, "[KERNEL] PASS: Kernel screen line-buffered double buffering implemented.") &&
        TextContains(debug_text, "[KERNEL] PASS: Kernel screen scroll proof keeps visible output stable.") &&
        TextContains(debug_text, "[KERNEL] PASS: Kernel screen visible presents are atomic.") &&
        TextContains(debug_text, "[KERNEL] PASS: Physical allocator tracking capacity is sufficient.") &&
        (interactive_display ?
            TextContains(debug_text, "[KERNEL] PASS: Interactive QEMU display mode keeps VM open for scroll testing.") :
            TextContains(debug_text, "[KERNEL] Requesting QEMU debug-exit success")) && command_ok;

    if (boot_pass)
    {
        free(debug_text);
        OrynLogOk(interactive_display ?
            "Boot proof passed after QEMU window was closed. Interactive screen scrolling test mode was active." :
            "Boot proof passed. Kernel output was captured and QEMU exited cleanly.");
        return 1;
    }

    free(debug_text);
    OrynLogFail("Boot proof failed. See Output/BootReport.txt and Output/Debug.log.");
    return 0;
}

int OrynCleanProject(const OrynProject* project)
{
    OrynLogStep("Cleaning project build/output folders.");
    OrynRemoveDirectoryRecursive(project->build_dir);
    OrynRemoveDirectoryRecursive(project->output_dir);
    OrynLogOk("Project cleaned.");
    return 1;
}
