#include "OrynBuild.h"
#include "CommandsSupport.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void WriteBootReport(
    const OrynProject* project,
    const char* report_path,
    const char* qemu_path,
    const char* ovmf_windows,
    const char* ovmf_qemu,
    const char* disk_windows,
    const char* debug_log,
    const char* command,
    const char* debug_text,
    int exit_code,
    int command_ok)
{
    int loader_started = TextContains(debug_text, "[BOOT] Stage 01");
    int kernel_loaded = TextContains(debug_text, "[BOOT] Kernel loaded physical base");
    int entry_printed = TextContains(debug_text, "[BOOT] Kernel virtual entry address");
    int virtual_map_prepared = TextContains(debug_text, "[BOOT] PASS: Temporary higher-half/chosen virtual map prepared");
    int virtual_map_active = TextContains(debug_text, "[BOOT] PASS: Temporary higher-half/chosen virtual map activated");
    int boot_services_exited = TextContains(debug_text, "[BOOT] Stage 07: ExitBootServices succeeded");
    int bootinfo_created = TextContains(debug_text, "[BOOT] BootInfo allocated at");
    int boot_config_prepared = TextContains(debug_text, "[BOOT] PASS: Boot configuration block prepared");
    int memory_map_requested = !TextContains(debug_text, "[BOOT] BootInfo selection memory map: disabled");
    int bootinfo_memory_map = !memory_map_requested || TextContains(debug_text, "[BOOT] BootInfo memory map entries");
    int kernel_jump = TextContains(debug_text, "[BOOT] Stage 08: Jumping to kernel entry");
    int kernel_entered = TextContains(debug_text, "[KERNEL] PASS: Kernel entered successfully");
    int serial_ok = TextContains(debug_text, "[KERNEL] PASS: Serial/debug output path is working");
    int bootinfo_received = TextContains(debug_text, "[KERNEL] PASS: BootInfo received");
    int kernel_boot_config = TextContains(debug_text, "[KERNEL] PASS: Boot configuration block received");
    int kernel_command_line = TextContains(debug_text, "[KERNEL] PASS: Kernel command line received");
    int gdt_installing = TextContains(debug_text, "[KERNEL] GDT: installing");
    int gdt_installed = TextContains(debug_text, "[KERNEL] PASS: GDT installed.");
    int gdt_entries = TextContains(debug_text, "[KERNEL] GDT entries: 7");
    int tss_loaded = TextContains(debug_text, "[KERNEL] PASS: TSS loaded.");
    int idt_installing = TextContains(debug_text, "[KERNEL] IDT: installing");
    int idt_installed = TextContains(debug_text, "[KERNEL] PASS: IDT installed.");
    int idt_entries = TextContains(debug_text, "[KERNEL] IDT entries: 256");
    int cpu_exception = TextContains(debug_text, "[KERNEL] EXCEPTION:");
    int cpu_local_apic = TextContains(debug_text, "[KERNEL] PASS: CPU local APIC feature present.");
    int cpu_apic2 = TextContains(debug_text, "[KERNEL] PASS: CPU APIC2/x2APIC feature present.");
    int pic_initialized = TextContains(debug_text, "[KERNEL] PASS: PIC initialized.");
    int pic_remapped = TextContains(debug_text, "[KERNEL] PASS: PIC remapped to vectors 0x20-0x2F.");
    int pic_masked = TextContains(debug_text, "[KERNEL] PASS: PIC masked/disabled for APIC handoff.");
    int apic_available = TextContains(debug_text, "[KERNEL] PASS: APIC CPU feature available.");
    int apic2_enabled = TextContains(debug_text, "[KERNEL] PASS: APIC2/x2APIC mode enabled.");
    int local_apic_enabled = TextContains(debug_text, "[KERNEL] PASS: Local APIC software enable bit set.");
    int apic_timer_probe = TextContains(debug_text, "[KERNEL] PASS: APIC timer counter moved in masked probe.");
    int hpet_rsdp = TextContains(debug_text, "[KERNEL] PASS: HPET ACPI RSDP input present.");
    int hpet_checksum = TextContains(debug_text, "[KERNEL] PASS: HPET ACPI checksum validation passed.");
    int hpet_table = TextContains(debug_text, "[KERNEL] PASS: HPET ACPI table discovered.");
    int hpet_enabled = TextContains(debug_text, "[KERNEL] PASS: HPET main counter enabled.");
    int hpet_counter = TextContains(debug_text, "[KERNEL] PASS: HPET counter advanced in probe.");
    int virtual_memory_started = TextContains(debug_text, "[KERNEL] Virtual memory: starting");
    int virtual_memory_required_mapped = TextContains(debug_text, "[KERNEL] Virtual memory: required ranges mapped");
    int virtual_memory_switching_cr3 = TextContains(debug_text, "[KERNEL] Virtual memory: switching CR3 to kernel-owned PML4");
    int virtual_memory_switched_cr3 = TextContains(debug_text, "[KERNEL] Virtual memory: CR3 switched to kernel-owned PML4");
    int virtual_memory_active = TextContains(debug_text, "[KERNEL] Virtual memory: active");
    int system_halted = TextContains(debug_text, "[KERNEL] System halted by Kernel-5");
    int debug_exit = TextContains(debug_text, "[KERNEL] Requesting QEMU debug-exit success");
    int boot_pass = command_ok && (exit_code == 0 || exit_code == 33) && loader_started &&
        kernel_loaded && entry_printed && virtual_map_prepared && virtual_map_active &&
        bootinfo_created && boot_config_prepared && bootinfo_memory_map && boot_services_exited && kernel_jump &&
        kernel_entered && serial_ok && bootinfo_received && kernel_boot_config && kernel_command_line &&
        gdt_installing && gdt_installed && gdt_entries && tss_loaded &&
        idt_installing && idt_installed && idt_entries && !cpu_exception &&
        cpu_local_apic && cpu_apic2 && pic_initialized && pic_remapped && pic_masked &&
        apic_available && apic2_enabled && local_apic_enabled && apic_timer_probe &&
        hpet_rsdp && hpet_checksum && hpet_table && hpet_enabled && hpet_counter && debug_exit;

    FILE* file = fopen(report_path, "wb");
    if (file == 0)
    {
        OrynLogWarn("Could not write Output/BootReport.txt.");
        return;
    }

    fprintf(file, "Oryn WSL SDK Boot Report\n");
    fprintf(file, "Version: %s\n", ORYN_VERSION);
    fprintf(file, "Project: %s\n", project->name);
    fprintf(file, "Target: %s\n", project->target);
    fprintf(file, "Toolchain: %s\n", project->toolchain);
    fprintf(file, "Result: %s\n", PassFail(boot_pass));
    fprintf(file, "QEMU exit code: %d\n\n", exit_code);

    fprintf(file, "Checks:\n");
    fprintf(file, "  QEMU command accepted: %s\n", PassFail(command_ok));
    fprintf(file, "  Loader started: %s\n", PassFail(loader_started));
    fprintf(file, "  Kernel loaded physical address printed: %s\n", PassFail(kernel_loaded));
    fprintf(file, "  Kernel virtual entry address printed: %s\n", PassFail(entry_printed));
    fprintf(file, "  Loader prepared temporary higher-half/chosen map: %s\n", PassFail(virtual_map_prepared));
    fprintf(file, "  Loader activated temporary higher-half/chosen map: %s\n", PassFail(virtual_map_active));
    fprintf(file, "  BootInfo allocated: %s\n", PassFail(bootinfo_created));
    fprintf(file, "  Loader prepared boot configuration block: %s\n", PassFail(boot_config_prepared));
    fprintf(file, "  BootInfo memory map requested: %s\n", PassFail(memory_map_requested));
    fprintf(file, "  BootInfo memory map captured or intentionally omitted: %s\n", PassFail(bootinfo_memory_map));
    fprintf(file, "  ExitBootServices succeeded: %s\n", PassFail(boot_services_exited));
    fprintf(file, "  Loader jumped to kernel: %s\n", PassFail(kernel_jump));
    fprintf(file, "  Kernel entered: %s\n", PassFail(kernel_entered));
    fprintf(file, "  Serial/debug output reached kernel: %s\n", PassFail(serial_ok));
    fprintf(file, "  Kernel received BootInfo: %s\n", PassFail(bootinfo_received));
    fprintf(file, "  Kernel received boot configuration block: %s\n", PassFail(kernel_boot_config));
    fprintf(file, "  Kernel received command line: %s\n", PassFail(kernel_command_line));
    fprintf(file, "  Kernel started GDT install: %s\n", PassFail(gdt_installing));
    fprintf(file, "  Kernel installed and verified GDT: %s\n", PassFail(gdt_installed));
    fprintf(file, "  Kernel installed all 7 GDT entries: %s\n", PassFail(gdt_entries));
    fprintf(file, "  Kernel loaded TSS: %s\n", PassFail(tss_loaded));
    fprintf(file, "  Kernel started IDT install: %s\n", PassFail(idt_installing));
    fprintf(file, "  Kernel installed and verified IDT: %s\n", PassFail(idt_installed));
    fprintf(file, "  Kernel installed all 256 IDT entries: %s\n", PassFail(idt_entries));
    fprintf(file, "  Kernel had no trapped CPU exception: %s\n", PassFail(!cpu_exception));
    fprintf(file, "  CPU local APIC feature present: %s\n", PassFail(cpu_local_apic));
    fprintf(file, "  CPU APIC2/x2APIC feature present: %s\n", PassFail(cpu_apic2));
    fprintf(file, "  PIC initialized: %s\n", PassFail(pic_initialized));
    fprintf(file, "  PIC remapped to 0x20-0x2F: %s\n", PassFail(pic_remapped));
    fprintf(file, "  PIC masked for APIC handoff: %s\n", PassFail(pic_masked));
    fprintf(file, "  APIC CPU feature available: %s\n", PassFail(apic_available));
    fprintf(file, "  APIC2/x2APIC mode enabled: %s\n", PassFail(apic2_enabled));
    fprintf(file, "  Local APIC software enabled: %s\n", PassFail(local_apic_enabled));
    fprintf(file, "  APIC timer masked probe counted down: %s\n", PassFail(apic_timer_probe));
    fprintf(file, "  HPET RSDP input present: %s\n", PassFail(hpet_rsdp));
    fprintf(file, "  HPET ACPI checksum validated: %s\n", PassFail(hpet_checksum));
    fprintf(file, "  HPET table discovered: %s\n", PassFail(hpet_table));
    fprintf(file, "  HPET main counter enabled: %s\n", PassFail(hpet_enabled));
    fprintf(file, "  HPET counter advanced: %s\n", PassFail(hpet_counter));
    fprintf(file, "  Kernel virtual memory started: %s\n", PassFail(virtual_memory_started));
    fprintf(file, "  Kernel virtual memory required ranges mapped: %s\n", PassFail(virtual_memory_required_mapped));
    fprintf(file, "  Kernel virtual memory CR3 switch requested: %s\n", PassFail(virtual_memory_switching_cr3));
    fprintf(file, "  Kernel virtual memory CR3 switch completed: %s\n", PassFail(virtual_memory_switched_cr3));
    fprintf(file, "  Kernel virtual memory active proof printed: %s\n", PassFail(virtual_memory_active));
    fprintf(file, "  Kernel reached halt path: %s\n", PassFail(system_halted));
    fprintf(file, "  Kernel requested QEMU debug-exit: %s\n", PassFail(debug_exit));

    fprintf(file, "\nFailure hint:\n  %s\n\n",
        boot_pass ?
            "No kernel-side failure detected by the boot-proof markers." :
        !loader_started ?
            "The loader did not start." :
        !kernel_jump ?
            "The loader did not reach the kernel jump." :
        !kernel_entered ?
            "The kernel did not print the entry success marker." :
        !kernel_boot_config ?
            "The kernel did not validate the boot configuration block." :
        !kernel_command_line ?
            "The kernel did not validate the command line." :
        !gdt_installing ?
            "The kernel did not start GDT installation." :
        !gdt_installed ?
            "The kernel did not verify the GDT install." :
        !gdt_entries ?
            "The kernel did not install the expected GDT entries." :
        !tss_loaded ?
            "The kernel did not load the TSS selector." :
        !idt_installing ?
            "The kernel did not start IDT installation." :
        !idt_installed ?
            "The kernel did not verify the IDT install." :
        !idt_entries ?
            "The kernel did not install all 256 IDT entries." :
        cpu_exception ?
            "The IDT trapped a CPU exception. Check the exception vector and register dump." :
        !cpu_local_apic ?
            "The CPU did not expose the local APIC feature." :
        !cpu_apic2 ?
            "QEMU did not expose APIC2/x2APIC. Check the -cpu qemu64,+x2apic option." :
        !pic_initialized ?
            "The kernel did not initialize the 8259 PIC path." :
        !pic_remapped ?
            "The kernel did not remap the PIC vectors to 0x20-0x2F." :
        !pic_masked ?
            "The kernel did not mask the PIC for APIC handoff." :
        !apic_available ?
            "The kernel did not reach APIC feature validation." :
        !apic2_enabled ?
            "The kernel did not enable APIC2/x2APIC mode." :
        !local_apic_enabled ?
            "The kernel did not set the local APIC software enable bit." :
        !apic_timer_probe ?
            "The APIC timer masked probe did not count down." :
        !hpet_rsdp ?
            "The kernel did not receive the ACPI RSDP needed for HPET discovery." :
        !hpet_checksum ?
            "The HPET ACPI table walk failed checksum validation." :
        !hpet_table ?
            "The HPET ACPI table was not discovered." :
        !hpet_enabled ?
            "The HPET main counter was not enabled." :
        !hpet_counter ?
            "The HPET counter did not advance during the probe." :
        !virtual_memory_started ?
            "The kernel stopped before virtual-memory initialization." :
        !virtual_memory_required_mapped ?
            "The kernel stopped while building required virtual-memory mappings." :
        !virtual_memory_switching_cr3 ?
            "The kernel stopped after mapping required ranges and before attempting the CR3 switch." :
        !virtual_memory_switched_cr3 ?
            "The kernel stopped during the CR3 switch to the kernel-owned PML4." :
        !system_halted ?
            "The kernel passed the CR3 switch but did not reach its halt path." :
            "The kernel halted but did not request QEMU debug-exit success.");

    fprintf(file, "Paths:\n");
    fprintf(file, "  QEMU: %s\n", qemu_path);
    fprintf(file, "  OVMF Windows: %s\n", ovmf_windows);
    fprintf(file, "  OVMF QEMU: %s\n", ovmf_qemu);
    fprintf(file, "  FAT32 disk image: %s\n", disk_windows);
    fprintf(file, "  Debug log: %s\n", debug_log);
    fprintf(file, "  Boot report: %s\n\n", report_path);

    fprintf(file, "Command:\n%s\n\n", command);
    fprintf(file, "Captured debug output:\n%s", debug_text[0] == 0 ? "[empty]\n" : debug_text);
    fclose(file);
}

int OrynCommandDoctor(const char* executable_path)
{
    (void)executable_path;
    OrynLogStep("Checking Oryn WSL development tools.");

    int ok = 1;
    PrintToolStatus("clang", "clang", &ok);
    PrintToolStatus("ld.lld", "ld.lld", &ok);
    PrintToolStatus("lld-link", "lld-link", &ok);
    PrintToolStatus("llvm-objcopy", "llvm-objcopy", &ok);
    PrintToolStatus("wslpath", "wslpath", &ok);
    PrintWindowsQemuStatus(&ok);

    char ovmf[ORYN_MAX_PATH];
    if (OrynFindOvmf(ovmf, sizeof(ovmf)))
    {
        char message[ORYN_MAX_PATH + 128];
        snprintf(message, sizeof(message), "OVMF found: %s", ovmf);
        OrynLogOk(message);
    }
    else
    {
        OrynLogFail("OVMF was not found. Expected Windows QEMU OVMF under C:/Program Files/qemu/share, WSL ovmf, or ORYN_OVMF_PATH.");
        ok = 0;
    }

    if (ok)
    {
        OrynLogOk("Doctor checks passed.");
        return 0;
    }

    OrynLogFail("Doctor checks failed.");
    return 1;
}

int OrynCommandBuild(const char* executable_path, const char* project_file)
{
    OrynProject project;
    if (!OrynLoadProject(executable_path, project_file, &project))
    {
        return 1;
    }

    OrynLogKeyValue("Project", project.name);
    OrynLogKeyValue("Target", project.target);
    OrynLogKeyValue("Toolchain", project.toolchain);

    return OrynBuildKernel(&project) ? 0 : 1;
}

int OrynCommandImage(const char* executable_path, const char* project_file)
{
    OrynProject project;
    if (!OrynLoadProject(executable_path, project_file, &project))
    {
        return 1;
    }

    if (!OrynBuildKernel(&project))
    {
        return 1;
    }

    return OrynBuildImage(&project) ? 0 : 1;
}

int OrynCommandRun(const char* executable_path, const char* project_file)
{
    OrynProject project;
    if (!OrynLoadProject(executable_path, project_file, &project))
    {
        return 1;
    }

    if (!OrynBuildKernel(&project))
    {
        return 1;
    }

    if (!OrynBuildImage(&project))
    {
        return 1;
    }

    return OrynRunQemu(&project) ? 0 : 1;
}

int OrynCommandClean(const char* executable_path, const char* project_file)
{
    OrynProject project;
    if (!OrynLoadProject(executable_path, project_file, &project))
    {
        return 1;
    }

    return OrynCleanProject(&project) ? 0 : 1;
}

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

    char image_name[256];
    snprintf(image_name, sizeof(image_name), "%s.img", project->name);
    OrynJoinPath(disk_image, sizeof(disk_image), project->output_dir, image_name);
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

    OrynMakeStageFilePath(stage_disk_image, sizeof(stage_disk_image), stage_root, project->name, ".img");
    OrynMakeStageFilePath(stage_debug_log, sizeof(stage_debug_log), stage_root, project->name, "-Debug.log");
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
    OrynLogKeyValue("Display", display_mode);

    char debug_log[ORYN_MAX_PATH];
    char boot_report[ORYN_MAX_PATH];
    OrynJoinPath(debug_log, sizeof(debug_log), project->output_dir, "Debug.log");
    OrynJoinPath(boot_report, sizeof(boot_report), project->output_dir, "BootReport.txt");
    remove(debug_log);
    remove(boot_report);

    OrynLogKeyValue("Serial", "live WSL terminal stdio");
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

    char command[ORYN_MAX_PATH * 8];
    snprintf(command, sizeof(command),
        "%s -machine q35,hpet=on -cpu qemu64,+x2apic -m 512M -drive %s -no-reboot -display %s "
        "-monitor none -serial stdio -debugcon %s -global isa-debugcon.iobase=0xe9 "
        "-device isa-debug-exit,iobase=0xf4,iosize=0x04 -drive %s",
        qemu_quoted,
        firmware_quoted,
        display_mode,
        debug_quoted,
        drive_quoted);

    int exit_code = -1;
    int command_ok = RunQemuAndGetExitCode(command, &exit_code);

    if (OrynFileExists(stage_debug_log))
    {
        OrynCopyFile(stage_debug_log, debug_log);
    }

    char debug_text[65536];
    ReadFileText(debug_log, debug_text, sizeof(debug_text));
    WriteBootReport(project, boot_report, qemu_path, ovmf_windows, ovmf_qemu, disk_windows,
        debug_log, command, debug_text, exit_code, command_ok);

    PrintFileIfPresent("QEMU debug output", debug_log);
    PrintFileIfPresent("Boot report", boot_report);

    int boot_pass = TextContains(debug_text, "[KERNEL] PASS: Kernel entered successfully") &&
        TextContains(debug_text, "[KERNEL] PASS: BootInfo received") &&
        TextContains(debug_text, "[KERNEL] PASS: GDT installed.") &&
        TextContains(debug_text, "[KERNEL] GDT entries: 7") &&
        TextContains(debug_text, "[KERNEL] PASS: TSS loaded.") &&
        TextContains(debug_text, "[KERNEL] PASS: IDT installed.") &&
        TextContains(debug_text, "[KERNEL] PASS: CPU APIC2/x2APIC feature present.") &&
        TextContains(debug_text, "[KERNEL] PASS: PIC masked/disabled for APIC handoff.") &&
        TextContains(debug_text, "[KERNEL] PASS: APIC2/x2APIC mode enabled.") &&
        TextContains(debug_text, "[KERNEL] PASS: APIC timer counter moved in masked probe.") &&
        TextContains(debug_text, "[KERNEL] PASS: HPET counter advanced in probe.") &&
        !TextContains(debug_text, "[KERNEL] EXCEPTION:") &&
        TextContains(debug_text, "[KERNEL] Requesting QEMU debug-exit success") && command_ok;

    if (boot_pass)
    {
        OrynLogOk("Boot proof passed. Kernel output was captured and QEMU exited cleanly.");
        return 1;
    }

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
