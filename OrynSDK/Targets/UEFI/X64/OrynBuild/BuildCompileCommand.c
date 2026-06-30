#include "TargetBuildInternal.h"
#include <stdio.h>

void BuildCompileCommand(
    const OrynProject* project,
    const char* source_file,
    const char* object_file,
    const char* dependency_file,
    char* command,
    size_t command_size)
{
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

    snprintf(command, command_size,
        "clang --target=x86_64-none-elf -ffreestanding -fno-stack-protector "
        "-fno-stack-check -fno-builtin -fno-pic -fno-pie -mno-red-zone -m64 "
        "-Wall -Wextra -MMD -MF \"%s\" "
        "-DORYN_VM_PIC=%d -DORYN_VM_APIC=%d -DORYN_VM_APIC2=%d "
        "-DORYN_VM_HPET=%d -DORYN_VM_SMP_CPUS=%u -DORYN_VM_INTERACTIVE_DISPLAY=%d "
        "%s-I\"%s\" -I\"%s\" -I\"%s\" -I\"%s\" -I\"%s\" -I\"%s\" -c \"%s\" -o \"%s\"",
        dependency_file,
        ProjectBoolEnabledBuild(project->run_pic, 1),
        ProjectBoolEnabledBuild(project->run_apic, 1),
        ProjectBoolEnabledBuild(project->run_apic2, 1),
        ProjectBoolEnabledBuild(project->run_hpet, 1),
        ProjectCpuCountBuild(project),
        ProjectDisplayIsInteractiveBuild(project),
        selection_include_argument,
        project->sdk_kernel_common_include_dir,
        project->sdk_kernel_target_include_dir,
        project->include_dir,
        libc_include,
        handoff_include,
        target_boot_include,
        source_file,
        object_file);
}
