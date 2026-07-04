#include "TargetBuildInternal.h"
#include <stdio.h>
#include <string.h>

void BuildKernelArchiveLinkCommand(const OrynProject* project, const OrynBuildArchivePlan* plan, char* command, size_t command_size)
{
    char linker_script[ORYN_MAX_PATH];
    char kernel_file_name[256];
    char kernel_elf[ORYN_MAX_PATH];

    OrynJoinPath(linker_script, sizeof(linker_script), project->sdk_root, "Targets/UEFI/X64/Kernel.ld");
    OrynMakeKernelElfFileName(kernel_file_name, sizeof(kernel_file_name), project->name);
    OrynJoinPath(kernel_elf, sizeof(kernel_elf), project->build_dir, kernel_file_name);

    snprintf(command, command_size,
        "ld.lld -nostdlib -static -z max-page-size=0x1000 "
        "--defsym=ORYN_KERNEL_PHYSICAL_BASE=0x%llX "
        "--defsym=ORYN_KERNEL_VIRTUAL_BASE=0x%llX "
        "-T \"%s\" -o \"%s\"",
        project->kernel_physical_base,
        project->kernel_virtual_base,
        linker_script,
        kernel_elf);

    AppendSelectedKernelModuleLinkRootFlags(project, command, command_size);
    strncat(command, " --whole-archive --start-group", command_size - strlen(command) - 1U);

    for (int order = 0; order < plan->ResolvedCount; ++order)
    {
        const OrynBuildModule* module = &plan->Modules[plan->ResolvedOrder[order]];
        if (module->Objects->count == 0)
        {
            char detail[256];
            snprintf(detail, sizeof(detail), "module=%s archive=%s", module->Name, module->ArchivePath);
            LogBuildPlanSkip(project, "link-archive-skipped-no-objects", detail);
            continue;
        }
        {
            char detail[ORYN_MAX_PATH + 160];
            snprintf(detail, sizeof(detail), "order=%d module=%s archive=%s", order, module->Name, module->ArchivePath);
            LogBuildPlanDecision(project, "link-archive-included", detail);
        }
        strncat(command, " \"", command_size - strlen(command) - 1U);
        strncat(command, module->ArchivePath, command_size - strlen(command) - 1U);
        strncat(command, "\"", command_size - strlen(command) - 1U);
    }

    strncat(command, " --end-group --no-whole-archive", command_size - strlen(command) - 1U);
}

int LinkKernelArchives(const OrynProject* project, const OrynBuildArchivePlan* plan)
{
    char command[ORYN_MAX_PATH * 8];
    BuildKernelArchiveLinkCommand(project, plan, command, sizeof(command));
    LogBuildPlanDecision(project, "link-command", command);
    if (!OrynRunCommand(command))
    {
        OrynLogFail("Kernel module/archive link failed.");
        return 0;
    }

    char kernel_file_name[256];
    char link_message[ORYN_MAX_PATH + 96];
    OrynMakeKernelElfFileName(kernel_file_name, sizeof(kernel_file_name), project->name);
    snprintf(link_message, sizeof(link_message), "Linked Build/%s from resolved module archives", kernel_file_name);
    LogBuildPlanDecision(project, "link-finished", link_message);
    OrynLogOk(link_message);
    return 1;
}
