#include "TargetBuildInternal.h"
#include <stdio.h>
#include <string.h>

static void RemoveExistingArchiveBeforeCreate(const OrynProject* project, const OrynBuildModule* module)
{
    if (OrynFileExists(module->ArchivePath))
    {
        char detail[ORYN_MAX_PATH + 160];
        snprintf(detail, sizeof(detail), "module=%s stale-archive=%s", module->Name, module->ArchivePath);
        LogBuildPlanDecision(project, "archive-remove-stale-before-create", detail);
        remove(module->ArchivePath);
    }
}

int ArchiveKernelModule(const OrynProject* project, OrynBuildModule* module)
{
    if (module->Objects->count == 0)
    {
        char detail[256];
        snprintf(detail, sizeof(detail), "module=%s archive=%s", module->Name, module->ArchivePath);
        LogBuildPlanSkip(project, "archive-skipped-no-objects", detail);
        return 1;
    }

    RemoveExistingArchiveBeforeCreate(project, module);

    char command[ORYN_MAX_PATH * 8];
    snprintf(command, sizeof(command), "llvm-ar rcs \"%s\"", module->ArchivePath);
    for (int index = 0; index < module->Objects->count; ++index)
    {
        strncat(command, " \"", sizeof(command) - strlen(command) - 1U);
        strncat(command, module->Objects->items[index], sizeof(command) - strlen(command) - 1U);
        strncat(command, "\"", sizeof(command) - strlen(command) - 1U);
    }

    char message[ORYN_MAX_PATH + 128];
    char detail[ORYN_MAX_PATH + 256];
    snprintf(detail, sizeof(detail), "module=%s objects=%d archive=%s", module->Name, module->Objects->count, module->ArchivePath);
    LogBuildPlanDecision(project, "archive-command", command);
    LogBuildPlanDecision(project, "archive-start", detail);
    snprintf(message, sizeof(message), "Archiving module %s -> %s", module->Name, module->ArchivePath);
    OrynLogStep(message);
    if (!OrynRunCommand(command))
    {
        snprintf(message, sizeof(message), "Module archive failed: %s", module->Name);
        LogBuildPlanSkip(project, "archive-command-failed", detail);
        OrynLogFail(message);
        return 0;
    }

    LogBuildPlanDecision(project, "archive-finished", detail);
    return 1;
}
