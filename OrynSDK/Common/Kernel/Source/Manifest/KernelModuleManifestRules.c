#include "KernelModuleManifest.h"
#include "KernelIo.h"
#include "KernelScreenReport.h"

OrynKernelModuleManifestItem* OrynKernelModuleManifestMutable(OrynKernelModuleId id);

int OrynKernelModuleManifestIsReady(OrynKernelModuleId id)
{
    const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet(id);
    return (item && item->State == OrynKernelModuleStateReady) ? 1 : 0;
}

int OrynKernelModuleManifestIsCompiledIn(OrynKernelModuleId id)
{
    const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet(id);
    return (item && item->CompiledIn) ? 1 : 0;
}

int OrynKernelModuleManifestIsRequired(OrynKernelModuleId id)
{
    const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet(id);
    return (item && item->Required) ? 1 : 0;
}

int OrynKernelModuleManifestFatalOnMissingPrerequisite(OrynKernelModuleId id)
{
    const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet(id);
    return (item && item->FatalOnMissingPrerequisite) ? 1 : 0;
}

const char* OrynKernelModuleManifestSelects(OrynKernelModuleId id)
{
    const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet(id);
    return item ? item->Selects : "";
}

static void ReportBlockedDependency(const OrynKernelModuleManifestItem* item)
{
    OrynKernelScreenReportBeginFail();
    KernelIoWriteString("Module blocked by dependency order: ");
    KernelIoWriteString(item ? item->Name : "unknown");
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
    if (!item || !item->CompiledIn)
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

void OrynKernelModuleManifestSelected(OrynKernelModuleId id)
{
    OrynKernelModuleManifestItem* item = OrynKernelModuleManifestMutable(id);
    if (item && item->State == OrynKernelModuleStateRegistered)
    {
        item->State = OrynKernelModuleStateSelected;
    }
}

int OrynKernelModuleManifestBegin(OrynKernelModuleId id)
{
    OrynKernelModuleManifestItem* item = OrynKernelModuleManifestMutable(id);
    if (!item || !item->CompiledIn)
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

int OrynKernelModuleManifestHasLifecycleCallbacks(OrynKernelModuleId id)
{
    const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet(id);
    return (item && item->StopCallback && item->PanicCallback && item->ShutdownCallback &&
        item->StopCallbackName && item->PanicCallbackName && item->ShutdownCallbackName) ? 1 : 0;
}

static int ShouldRunLifecycleCallback(const OrynKernelModuleManifestItem* item)
{
    if (!item || !item->CompiledIn)
    {
        return 0;
    }

    if (item->State == OrynKernelModuleStateAbsent || item->State == OrynKernelModuleStateSkipped ||
        item->State == OrynKernelModuleStateStopped || item->State == OrynKernelModuleStateShutdown)
    {
        return 0;
    }

    return 1;
}

int OrynKernelModuleManifestStop(OrynKernelModuleId id)
{
    OrynKernelModuleManifestItem* item = OrynKernelModuleManifestMutable(id);
    if (!ShouldRunLifecycleCallback(item))
    {
        return 0;
    }

    item->State = OrynKernelModuleStateStopping;
    if (!item->StopCallback || !item->StopCallback(id))
    {
        item->State = OrynKernelModuleStateFailed;
        return 0;
    }

    item->State = OrynKernelModuleStateStopped;
    return 1;
}

int OrynKernelModuleManifestPanic(OrynKernelModuleId id)
{
    OrynKernelModuleManifestItem* item = OrynKernelModuleManifestMutable(id);
    if (!ShouldRunLifecycleCallback(item))
    {
        return 0;
    }

    item->State = OrynKernelModuleStatePanic;
    if (!item->PanicCallback || !item->PanicCallback(id))
    {
        item->State = OrynKernelModuleStateFailed;
        return 0;
    }

    return 1;
}

int OrynKernelModuleManifestShutdown(OrynKernelModuleId id)
{
    OrynKernelModuleManifestItem* item = OrynKernelModuleManifestMutable(id);
    if (!ShouldRunLifecycleCallback(item))
    {
        return 0;
    }

    item->State = OrynKernelModuleStateShuttingDown;
    if (!item->ShutdownCallback || !item->ShutdownCallback(id))
    {
        item->State = OrynKernelModuleStateFailed;
        return 0;
    }

    item->State = OrynKernelModuleStateShutdown;
    return 1;
}

unsigned int OrynKernelModuleManifestInvokeStopCallbacks(void)
{
    unsigned int count = 0U;
    for (int index = (int)OrynKernelModuleCount - 1; index >= 0; --index)
    {
        if (OrynKernelModuleManifestStop((OrynKernelModuleId)index))
        {
            ++count;
        }
    }
    return count;
}

unsigned int OrynKernelModuleManifestInvokePanicCallbacks(void)
{
    unsigned int count = 0U;
    for (int index = (int)OrynKernelModuleCount - 1; index >= 0; --index)
    {
        if (OrynKernelModuleManifestPanic((OrynKernelModuleId)index))
        {
            ++count;
        }
    }
    return count;
}

unsigned int OrynKernelModuleManifestInvokeShutdownCallbacks(void)
{
    unsigned int count = 0U;
    for (int index = (int)OrynKernelModuleCount - 1; index >= 0; --index)
    {
        if (OrynKernelModuleManifestShutdown((OrynKernelModuleId)index))
        {
            ++count;
        }
    }
    return count;
}

const char* OrynKernelModuleManifestStateName(OrynKernelModuleState state)
{
    switch (state)
    {
        case OrynKernelModuleStateAbsent: return "absent";
        case OrynKernelModuleStateRegistered: return "registered";
        case OrynKernelModuleStateSelected: return "selected";
        case OrynKernelModuleStateStarting: return "starting";
        case OrynKernelModuleStateReady: return "ready";
        case OrynKernelModuleStateStopping: return "stopping";
        case OrynKernelModuleStateStopped: return "stopped";
        case OrynKernelModuleStatePanic: return "panic";
        case OrynKernelModuleStateShuttingDown: return "shutting-down";
        case OrynKernelModuleStateShutdown: return "shutdown";
        case OrynKernelModuleStateSkipped: return "skipped";
        case OrynKernelModuleStateFailed: return "failed";
        default: return "unknown";
    }
}
