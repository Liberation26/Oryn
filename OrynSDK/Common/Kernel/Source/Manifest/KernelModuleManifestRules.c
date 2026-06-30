#include "KernelModuleManifest.h"
#include "KernelIo.h"
#include "KernelScreenReport.h"

OrynKernelModuleManifestItem* OrynKernelModuleManifestMutable(OrynKernelModuleId id);

int OrynKernelModuleManifestIsReady(OrynKernelModuleId id)
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


unsigned int OrynKernelModuleManifestRequireCount(OrynKernelModuleId id)
{
    const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet(id);
    return item ? item->RequireCount : 0U;
}

OrynKernelModuleId OrynKernelModuleManifestRequireAt(OrynKernelModuleId id, unsigned int index)
{
    const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet(id);
    if (!item || index >= item->RequireCount)
    {
        return OrynKernelModuleCount;
    }

    return item->Requires[index];
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
        if (!OrynKernelModuleManifestIsReady(item->Requires[index]))
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

void OrynKernelModuleManifestSkipped(OrynKernelModuleId id)
{
    OrynKernelModuleManifestItem* item = OrynKernelModuleManifestMutable(id);
    if (item)
    {
        item->State = OrynKernelModuleStateSkipped;
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

const char* OrynKernelModuleManifestStateName(OrynKernelModuleState state)
{
    switch (state)
    {
        case OrynKernelModuleStateAbsent: return "absent";
        case OrynKernelModuleStateRegistered: return "registered";
        case OrynKernelModuleStateStarting: return "starting";
        case OrynKernelModuleStateReady: return "ready";
        case OrynKernelModuleStateSkipped: return "skipped";
        case OrynKernelModuleStateFailed: return "failed";
        default: return "unknown";
    }
}
