#include "KernelDiagnosticsProofsInternal.h"
#include "KernelModuleManifest.h"
#include "OrynString.h"

static int HasBootFlag(const OrynBootInfo* bootInfo, unsigned long long flag)
{
    return (bootInfo && KernelBootInfoHasFlag(bootInfo, flag)) ? 1 : 0;
}

static int SelectionHasToken(const char* selection, const char* token)
{
    const char* cursor = selection;
    unsigned int tokenLength = 0U;
    while (token[tokenLength] != 0)
    {
        ++tokenLength;
    }

    while (cursor && *cursor != 0)
    {
        while (*cursor == ',' || *cursor == ' ' || *cursor == '\t')
        {
            ++cursor;
        }

        const char* start = cursor;
        while (*cursor != 0 && *cursor != ',')
        {
            ++cursor;
        }

        if ((unsigned int)(cursor - start) == tokenLength && strncmp(start, token, tokenLength) == 0)
        {
            return 1;
        }
    }

    return 0;
}

static int SelectionTokenAllowed(const OrynBootInfo* bootInfo, const char* token, const char** reason)
{
    if (strcmp(token, "Always") == 0)
    {
        return 1;
    }
    if (strcmp(token, "VmPic") == 0)
    {
#if ORYN_VM_PIC
        return 1;
#else
        *reason = "PIC was not selected by the project VM profile";
        return 0;
#endif
    }
    if (strcmp(token, "VmApic") == 0)
    {
#if ORYN_VM_APIC || ORYN_VM_APIC2
        return 1;
#else
        *reason = "APIC/APIC2 was not selected by the project VM profile";
        return 0;
#endif
    }
    if (strcmp(token, "VmSmp") == 0)
    {
#if ORYN_VM_SMP_CPUS > 1
        return 1;
#else
        *reason = "SMP has no selected extra CPU to start";
        return 0;
#endif
    }
    if (strcmp(token, "VmHpet") == 0)
    {
#if ORYN_VM_HPET
        return 1;
#else
        *reason = "HPET was not selected by the project VM profile";
        return 0;
#endif
    }
    if (strcmp(token, "BootInfoFramebuffer") == 0)
    {
        if (HasBootFlag(bootInfo, ORYN_BOOTINFO_FLAG_FRAMEBUFFER))
        {
            return 1;
        }
        *reason = "framebuffer was not supplied by BootInfo";
        return 0;
    }
    if (strcmp(token, "BootInfoRsdp") == 0)
    {
        if (HasBootFlag(bootInfo, ORYN_BOOTINFO_FLAG_RSDP))
        {
            return 1;
        }
        *reason = "ACPI RSDP was not supplied by BootInfo";
        return 0;
    }
    if (strcmp(token, "BootInfoMemoryMap") == 0)
    {
        if (HasBootFlag(bootInfo, ORYN_BOOTINFO_FLAG_MEMORY_MAP))
        {
            return 1;
        }
        *reason = "memory map was not supplied by BootInfo";
        return 0;
    }
    if (strcmp(token, "BootInfoKernelRange") == 0)
    {
        if (HasBootFlag(bootInfo, ORYN_BOOTINFO_FLAG_KERNEL_RANGE))
        {
            return 1;
        }
        *reason = "kernel range was not supplied by BootInfo";
        return 0;
    }

    *reason = "unknown manifest selection token";
    return 0;
}

static int ModuleInputSelected(const OrynBootInfo* bootInfo, OrynKernelModuleId id, const char** reason)
{
    const char* selection = OrynKernelModuleManifestSelects(id);
    char token[64];
    unsigned int tokenLength = 0U;
    const char* cursor = selection;

    if (!selection || selection[0] == 0)
    {
        *reason = "no manifest selection tokens";
        return 0;
    }

    if (SelectionHasToken(selection, "Always"))
    {
        return 1;
    }

    while (*cursor != 0)
    {
        tokenLength = 0U;
        while (*cursor == ',' || *cursor == ' ' || *cursor == '\t')
        {
            ++cursor;
        }
        while (*cursor != 0 && *cursor != ',' && tokenLength + 1U < sizeof(token))
        {
            token[tokenLength++] = *cursor++;
        }
        token[tokenLength] = 0;
        if (token[0] != 0 && !SelectionTokenAllowed(bootInfo, token, reason))
        {
            return 0;
        }
    }

    return 1;
}

static const char* MissingDependencyReason(OrynKernelModuleId id)
{
    static const char* fallback = "a prerequisite module is not ready";
    for (unsigned int index = 0U; index < OrynKernelModuleManifestRequireCount(id); ++index)
    {
        OrynKernelModuleId required = OrynKernelModuleManifestRequireAt(id, index);
        if (!OrynKernelModuleManifestIsReady(required))
        {
            const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet(required);
            return item ? item->Name : fallback;
        }
    }
    return fallback;
}

static void ReportModuleWarn(OrynKernelModuleId id, const char* prefix, const char* reason)
{
    const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet(id);
    OrynKernelScreenReportBeginWarn();
    OrynKernelDiagnosticsLogText(prefix);
    OrynKernelDiagnosticsLogText(item ? item->Name : "unknown");
    OrynKernelDiagnosticsLogText(" because ");
    OrynKernelDiagnosticsLogText(reason ? reason : "nothing was selected to start");
    OrynKernelDiagnosticsLogText(".\n");
}

static void ReportModuleFail(OrynKernelModuleId id, const char* prefix, const char* reason)
{
    const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet(id);
    OrynKernelScreenReportBeginFail();
    OrynKernelDiagnosticsLogText(prefix);
    OrynKernelDiagnosticsLogText(item ? item->Name : "unknown");
    OrynKernelDiagnosticsLogText(" because ");
    OrynKernelDiagnosticsLogText(reason ? reason : "a required prerequisite is missing");
    OrynKernelDiagnosticsLogText(".\n");
}

int OrynKernelDiagnosticsShouldStartModule(const OrynBootInfo* bootInfo, OrynKernelModuleId id)
{
    const char* reason = 0;
    if (!OrynKernelModuleManifestIsCompiledIn(id))
    {
        OrynKernelModuleManifestFailed(id);
        ReportModuleFail(id, "Required module not compiled in: ", "compiled-in registry does not contain it");
        return 0;
    }

    if (!ModuleInputSelected(bootInfo, id, &reason))
    {
        OrynKernelModuleManifestSkipped(id);
        if (OrynKernelModuleManifestIsRequired(id))
        {
            ReportModuleFail(id, "Required module has no selected boot input: ", reason);
        }
        else
        {
            ReportModuleWarn(id, "Optional module has nothing selected to start: ", reason);
        }
        return 0;
    }

    OrynKernelModuleManifestSelected(id);

    if (!OrynKernelModuleManifestCanStart(id))
    {
        reason = MissingDependencyReason(id);
        if (OrynKernelModuleManifestFatalOnMissingPrerequisite(id))
        {
            OrynKernelModuleManifestFailed(id);
            ReportModuleFail(id, "Selected module missing prerequisite: ", reason);
        }
        else
        {
            OrynKernelModuleManifestSkipped(id);
            ReportModuleWarn(id, "Selected optional module skipped: ", reason);
        }
        return 0;
    }

    return OrynKernelModuleManifestBegin(id);
}

void OrynKernelDiagnosticsPrintBootOptionPlan(const OrynBootInfo* bootInfo)
{
    (void)bootInfo;
    OrynKernelScreenReportOk(0, "Kernel boot option plan starts strictly from supplied BootInfo and project-selected options.");
    OrynKernelScreenReportOk(0, "Optional modules with no selected start input emit WARN instead of FAIL.");
    OrynKernelScreenReportOk(0, "Selected modules use manifest fatal/non-fatal prerequisite policy.");
}
