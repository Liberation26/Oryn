#include "KernelDiagnosticsProofsInternal.h"
#include "KernelModuleManifest.h"

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

static void ReportModuleSkipped(OrynKernelModuleId id, const char* reason)
{
    const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet(id);
    OrynKernelScreenReportBeginWarn();
    KernelIoWriteString("Module skipped by boot option plan: ");
    KernelIoWriteString(item ? item->Name : "unknown");
    KernelIoWriteString(" needs ");
    KernelIoWriteString(reason ? reason : "a dependency");
    KernelIoWriteString(".\n");
}

int OrynKernelDiagnosticsShouldStartModule(const OrynBootInfo* bootInfo, OrynKernelModuleId id)
{
    const char* reason = 0;
    if (!ModuleVmProfileAllows(id, &reason) || !ModuleBootInfoAllows(bootInfo, id, &reason))
    {
        OrynKernelModuleManifestSkipped(id);
        ReportModuleSkipped(id, reason);
        return 0;
    }

    if (!OrynKernelModuleManifestCanStart(id))
    {
        OrynKernelModuleManifestSkipped(id);
        ReportModuleSkipped(id, "a ready prerequisite module");
        return 0;
    }

    return 1;
}

void OrynKernelDiagnosticsPrintBootOptionPlan(const OrynBootInfo* bootInfo)
{
    (void)bootInfo;
    OrynKernelScreenReportOk(0, "Kernel boot option plan uses supplied BootInfo and module needs.");
}
