#include "OrynBuild.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

static void PrintToolStatus(const char* label, const char* program, int* ok)
{
    char path[ORYN_MAX_PATH];
    if (OrynFindProgram(program, path, sizeof(path)))
    {
        char message[ORYN_MAX_PATH + 128];
        snprintf(message, sizeof(message), "%s found: %s", label, path);
        OrynLogOk(message);
    }
    else
    {
        char message[256];
        snprintf(message, sizeof(message), "%s was not found: %s", label, program);
        OrynLogFail(message);
        *ok = 0;
    }
}

static void PrintWindowsQemuStatus(int* ok)
{
    char qemu[ORYN_MAX_PATH];
    if (OrynFindWindowsQemu(qemu, sizeof(qemu)))
    {
        char message[ORYN_MAX_PATH + 128];
        snprintf(message, sizeof(message), "Windows QEMU found: %s", qemu);
        OrynLogOk(message);
        return;
    }

    OrynLogFail("Windows QEMU was not found from WSL.");
    OrynLogWarn("Expected default Windows QEMU path: /mnt/c/Program Files/qemu/qemu-system-x86_64.exe");
    OrynLogWarn("Optional override: ORYN_QEMU_WINDOWS_PATH");
    *ok = 0;
}

static void PrintFileIfPresent(const char* title, const char* path)
{
    FILE* file = fopen(path, "rb");
    if (file == 0)
    {
        return;
    }

    printf("\n[INFO] %s: %s\n", title, path);
    printf("-------- %s --------\n", title);

    int value;
    int wrote_any = 0;
    while ((value = fgetc(file)) != EOF)
    {
        putchar(value);
        wrote_any = 1;
    }

    if (!wrote_any)
    {
        printf("[empty]\n");
    }
    else
    {
        printf("\n");
    }

    printf("------ end %s ------\n", title);
    fclose(file);
}

static int ReadFileText(const char* path, char* output, size_t output_size)
{
    output[0] = 0;
    FILE* file = fopen(path, "rb");
    if (file == 0)
    {
        return 0;
    }

    size_t used = fread(output, 1, output_size - 1, file);
    output[used] = 0;
    fclose(file);
    return 1;
}

static int TextContains(const char* text, const char* needle)
{
    return strstr(text, needle) != 0;
}

static const char* PassFail(int value)
{
    return value ? "PASS" : "FAIL";
}

static int TextEqualsIgnoreCase(const char* left, const char* right)
{
    if (left == 0 || right == 0)
    {
        return 0;
    }

    while (*left != 0 && *right != 0)
    {
        char a = *left;
        char b = *right;
        if (a >= 'A' && a <= 'Z')
        {
            a = (char)(a - 'A' + 'a');
        }
        if (b >= 'A' && b <= 'Z')
        {
            b = (char)(b - 'A' + 'a');
        }
        if (a != b)
        {
            return 0;
        }
        ++left;
        ++right;
    }

    return *left == 0 && *right == 0;
}

static const char* ResolveQemuDisplayMode(const OrynProject* project)
{
    if (project->run_display[0] == 0)
    {
        return "none";
    }

    if (TextEqualsIgnoreCase(project->run_display, "headless") ||
        TextEqualsIgnoreCase(project->run_display, "yes") ||
        TextEqualsIgnoreCase(project->run_display, "true"))
    {
        return "none";
    }

    if (TextEqualsIgnoreCase(project->run_display, "window") ||
        TextEqualsIgnoreCase(project->run_display, "windowed") ||
        TextEqualsIgnoreCase(project->run_display, "no") ||
        TextEqualsIgnoreCase(project->run_display, "false"))
    {
        return "sdl";
    }

    return project->run_display;
}

static int IsSafeQemuDisplayMode(const char* display)
{
    if (display == 0 || display[0] == 0)
    {
        return 0;
    }

    for (int index = 0; display[index] != 0; ++index)
    {
        char ch = display[index];
        if (!((ch >= 'A' && ch <= 'Z') ||
              (ch >= 'a' && ch <= 'z') ||
              (ch >= '0' && ch <= '9') ||
              ch == '_' || ch == '-' || ch == ',' || ch == '=' || ch == ':'))
        {
            return 0;
        }
    }

    return 1;
}

static int RunQemuAndGetExitCode(const char* command, int* exit_code)
{
    *exit_code = -1;
    printf("[CMD ] %s\n", command);

    int code = system(command);
    if (code == -1)
    {
        return 0;
    }

    if (!WIFEXITED(code))
    {
        return 0;
    }

    *exit_code = WEXITSTATUS(code);
    if (*exit_code == 0 || *exit_code == 33)
    {
        return 1;
    }

    printf("[WARN] QEMU exited with code %d.\n", *exit_code);
    return 0;
}

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
    int memory_map_requested = !TextContains(debug_text, "[BOOT] BootInfo selection memory map: disabled");
    int bootinfo_memory_map = !memory_map_requested || TextContains(debug_text, "[BOOT] BootInfo memory map entries");
    int kernel_jump = TextContains(debug_text, "[BOOT] Stage 08: Jumping to kernel entry");
    int kernel_entered = TextContains(debug_text, "[KERNEL] PASS: Kernel entered successfully");
    int serial_ok = TextContains(debug_text, "[KERNEL] PASS: Serial/debug output path is working");
    int bootinfo_received = TextContains(debug_text, "[KERNEL] PASS: BootInfo received");
    int debug_exit = TextContains(debug_text, "[KERNEL] Requesting QEMU debug-exit success");
    int boot_pass = command_ok && (exit_code == 0 || exit_code == 33) && loader_started &&
        kernel_loaded && entry_printed && virtual_map_prepared && virtual_map_active &&
        bootinfo_created && bootinfo_memory_map && boot_services_exited && kernel_jump &&
        kernel_entered && serial_ok && bootinfo_received && debug_exit;

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
    fprintf(file, "  BootInfo memory map requested: %s\n", PassFail(memory_map_requested));
    fprintf(file, "  BootInfo memory map captured or intentionally omitted: %s\n", PassFail(bootinfo_memory_map));
    fprintf(file, "  ExitBootServices succeeded: %s\n", PassFail(boot_services_exited));
    fprintf(file, "  Loader jumped to kernel: %s\n", PassFail(kernel_jump));
    fprintf(file, "  Kernel entered: %s\n", PassFail(kernel_entered));
    fprintf(file, "  Serial/debug output reached kernel: %s\n", PassFail(serial_ok));
    fprintf(file, "  Kernel received BootInfo: %s\n", PassFail(bootinfo_received));
    fprintf(file, "  Kernel requested QEMU debug-exit: %s\n\n", PassFail(debug_exit));

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
        "%s -machine q35 -m 512M -drive %s -no-reboot -display %s "
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
