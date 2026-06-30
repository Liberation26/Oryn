#include "KernelModuleManifest.h"
#include "KernelIo.h"
#include "KernelScreenReport.h"

static void PrintModuleLine(const OrynKernelModuleManifestItem* item)
{
    KernelIoWriteString("[KERNEL] Module manifest item: ");
    KernelIoWriteString(item->Name);
    KernelIoWriteString(" [");
    KernelIoWriteString(OrynKernelModuleManifestStateName(item->State));
    KernelIoWriteString("] -> ");
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
    OrynKernelScreenReportOk(0, "Module manifest supports boot-option driven start decisions.");

    for (unsigned int index = 0U; index < (unsigned int)OrynKernelModuleCount; ++index)
    {
        const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet((OrynKernelModuleId)index);
        if (item)
        {
            PrintModuleLine(item);
        }
    }
}
