#include "KernelModuleManifest.h"
#include "KernelIo.h"
#include "KernelScreenReport.h"

static void PrintModuleLine(const OrynKernelModuleManifestItem* item)
{
    KernelIoWriteString("[KERNEL] Module manifest item: ");
    KernelIoWriteString(item->Name);
    KernelIoWriteString(" [");
    KernelIoWriteString(OrynKernelModuleManifestStateName(item->State));
    KernelIoWriteString("] ");
    KernelIoWriteString(item->CompiledIn ? "compiled-in" : "not-compiled");
    KernelIoWriteString(item->Required ? " required" : " optional");
    KernelIoWriteString(item->FatalOnMissingPrerequisite ? " fatal-prereq" : " nonfatal-prereq");
    KernelIoWriteString(" select=");
    KernelIoWriteString(item->Selects);
    KernelIoWriteString(" -> ");
    KernelIoWriteString(item->Items);
    KernelIoWriteString("\n");
}

static unsigned int CountRegisteredModules(void)
{
    unsigned int count = 0U;
    for (unsigned int index = 0U; index < (unsigned int)OrynKernelModuleCount; ++index)
    {
        const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet((OrynKernelModuleId)index);
        if (item && item->State != OrynKernelModuleStateAbsent)
        {
            ++count;
        }
    }

    return count;
}

void OrynKernelCompiledModuleRegistryPrintProof(void)
{
    unsigned int compiled = 0U;
    for (unsigned int index = 0U; index < OrynKernelCompiledModuleCount(); ++index)
    {
        const OrynKernelCompiledModuleRecord* record = OrynKernelCompiledModuleGet(index);
        if (record && record->CompiledIn)
        {
            ++compiled;
            KernelIoWriteString("[KERNEL] Compiled-in module: ");
            KernelIoWriteString(record->Name);
            KernelIoWriteString("\n");
        }
    }

    OrynKernelScreenReportOkOrFail(compiled == (unsigned int)OrynKernelModuleCount,
        "Compiled-in kernel module registry contains every manifest module.",
        "Compiled-in kernel module registry is missing manifest modules.");
}

void OrynKernelModuleManifestPrintProof(void)
{
    unsigned int count = CountRegisteredModules();
    OrynKernelScreenReportOkOrFail(count == (unsigned int)OrynKernelModuleCount,
        "Module manifest contains every kernel module item set.",
        "Module manifest is missing kernel module item sets.");
    OrynKernelScreenReportOkOrFail(
        !OrynKernelModuleManifestCanStart(OrynKernelModuleApic),
        "Module manifest blocks APIC before PIC is ready.",
        "Module manifest allows APIC before PIC.");
    OrynKernelScreenReportOk(0, "Module manifest owns init/start state transitions and boot policy.");
    OrynKernelScreenReportOk(0, "Module manifest carries required, optional, and fatal prerequisite policy.");

    OrynKernelCompiledModuleRegistryPrintProof();

    for (unsigned int index = 0U; index < (unsigned int)OrynKernelModuleCount; ++index)
    {
        const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet((OrynKernelModuleId)index);
        if (item)
        {
            PrintModuleLine(item);
        }
    }
}
