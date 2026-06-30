#include "KernelModuleManifest.h"
#include "KernelIo.h"
#include "KernelScreenReport.h"

OrynKernelModuleManifestItem* OrynKernelModuleManifestMutable(OrynKernelModuleId id);

static int IsReady(OrynKernelModuleId id)
{
    const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet(id);
    return (item && item->State == OrynKernelModuleStateReady) ? 1 : 0;
}

static void ReportBlockedDependency(const OrynKernelModuleManifestItem* item)
{
    OrynKernelScreenReportBeginFail();
    KernelIoWriteString("Module blocked by dependency order: ");
    KernelIoWriteString(item->Name);
    KernelIoWriteString(".");
    KernelIoWriteString("\n");
}

int OrynKernelModuleManifestCanStart(OrynKernelModuleId id)
{
    const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet(id);
    if (!item)
    {
        return 0;
    }

    for (unsigned int index = 0U; index < item->RequireCount; ++index)
    {
        if (!IsReady(item->Requires[index]))
        {
            return 0;
        }
    }

    return 1;
}

int OrynKernelModuleManifestBegin(OrynKernelModuleId id)
{
    OrynKernelModuleManifestItem* item = OrynKernelModuleManifestMutable(id);
    if (!item)
    {
        return 0;
    }

    if (!OrynKernelModuleManifestCanStart(id))
    {
        item->State = OrynKernelModuleStateFailed;
        ReportBlockedDependency(item);
        return 0;
    }

    item->State = OrynKernelModuleStateStarting;
    return 1;
}

void OrynKernelModuleManifestReady(OrynKernelModuleId id)
{
    OrynKernelModuleManifestItem* item = OrynKernelModuleManifestMutable(id);
    if (item)
    {
        item->State = OrynKernelModuleStateReady;
    }
}

void OrynKernelModuleManifestFailed(OrynKernelModuleId id)
{
    OrynKernelModuleManifestItem* item = OrynKernelModuleManifestMutable(id);
    if (item)
    {
        item->State = OrynKernelModuleStateFailed;
    }
}
