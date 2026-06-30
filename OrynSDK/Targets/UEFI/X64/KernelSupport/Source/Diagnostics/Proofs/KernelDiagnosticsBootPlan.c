#include "KernelDiagnosticsProofsInternal.h"
#include "KernelModuleManifest.h"

#define ORYN_BOOT_PLAN_RECURSION_LIMIT 8U

static int HasBootFlag(const OrynBootInfo* bootInfo, unsigned long long flag)
{
    return (bootInfo && KernelBootInfoHasFlag(bootInfo, flag)) ? 1 : 0;
}

static int ModuleVmProfileAllows(OrynKernelModuleId id, const char** reason)
{
    if (id == OrynKernelModulePic && !ORYN_VM_PIC)
    {
        *reason = "PIC disabled by VM profile";
        return 0;
    }
    if (id == OrynKernelModuleApic && !(ORYN_VM_APIC || ORYN_VM_APIC2))
    {
        *reason = "APIC disabled by VM profile";
        return 0;
    }
    if (id == OrynKernelModuleSmp && (ORYN_VM_SMP_CPUS <= 1 || !(ORYN_VM_APIC || ORYN_VM_APIC2)))
    {
        *reason = "SMP disabled by VM profile";
        return 0;
    }
    if (id == OrynKernelModuleHpet && !ORYN_VM_HPET)
    {
        *reason = "HPET disabled by VM profile";
        return 0;
    }

    return 1;
}

static int ModuleBootInfoAllows(const OrynBootInfo* bootInfo, OrynKernelModuleId id, const char** reason)
{
    if ((id == OrynKernelModuleConsole || id == OrynKernelModuleKeyboard) &&
        !HasBootFlag(bootInfo, ORYN_BOOTINFO_FLAG_FRAMEBUFFER))
    {
        *reason = "framebuffer was not supplied";
        return 0;
    }
    if ((id == OrynKernelModulePci || id == OrynKernelModuleHpet || id == OrynKernelModuleSmp) &&
        !HasBootFlag(bootInfo, ORYN_BOOTINFO_FLAG_RSDP))
    {
        *reason = "ACPI RSDP was not supplied";
        return 0;
    }
    if ((id == OrynKernelModulePhysicalMemory || id == OrynKernelModuleVirtualMemory) &&
        !HasBootFlag(bootInfo, ORYN_BOOTINFO_FLAG_MEMORY_MAP))
    {
        *reason = "memory map was not supplied";
        return 0;
    }
    if (id == OrynKernelModuleVirtualMemory &&
        !HasBootFlag(bootInfo, ORYN_BOOTINFO_FLAG_KERNEL_RANGE))
    {
        *reason = "kernel range was not supplied";
        return 0;
    }

    return 1;
}

static int ModuleInputsAllow(const OrynBootInfo* bootInfo, OrynKernelModuleId id, const char** reason)
{
    return ModuleVmProfileAllows(id, reason) && ModuleBootInfoAllows(bootInfo, id, reason);
}

static int ModuleCanBeFilled(const OrynBootInfo* bootInfo, OrynKernelModuleId id,
    unsigned int depth, const char** reason)
{
    unsigned int requireCount;
    if (depth > ORYN_BOOT_PLAN_RECURSION_LIMIT)
    {
        *reason = "recursive prerequisite depth limit";
        return 0;
    }
    if (!ModuleInputsAllow(bootInfo, id, reason))
    {
        return 0;
    }
    if (OrynKernelModuleManifestIsReady(id))
    {
        return 1;
    }

    requireCount = OrynKernelModuleManifestRequireCount(id);
    for (unsigned int index = 0U; index < requireCount; ++index)
    {
        OrynKernelModuleId required = OrynKernelModuleManifestRequireAt(id, index);
        if (!OrynKernelModuleManifestIsReady(required) &&
            !ModuleCanBeFilled(bootInfo, required, depth + 1U, reason))
        {
            return 0;
        }
    }

    return 1;
}

static void ReportModuleSkipped(OrynKernelModuleId id, const char* reason)
{
    const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet(id);
    OrynKernelScreenReportBeginWarn();
    KernelIoWriteString("Module skipped by boot option plan: ");
    KernelIoWriteString(item ? item->Name : "unknown");
    KernelIoWriteString(" needs ");
    KernelIoWriteString(reason ? reason : "a prerequisite module");
    KernelIoWriteString(".\n");
}

int OrynKernelDiagnosticsShouldStartModule(const OrynBootInfo* bootInfo, OrynKernelModuleId id)
{
    const char* reason = 0;
    if (!ModuleCanBeFilled(bootInfo, id, 0U, &reason))
    {
        OrynKernelModuleManifestSkipped(id);
        ReportModuleSkipped(id, reason);
        return 0;
    }

    return 1;
}

void OrynKernelDiagnosticsPrintBootOptionPlan(const OrynBootInfo* bootInfo)
{
    (void)bootInfo;
    OrynKernelScreenReportOk(0, "Kernel boot option plan uses supplied BootInfo and module needs.");
    OrynKernelScreenReportOk(0, "Kernel prerequisite filler resolves startable dependencies before warning.");
}
