#include "KernelModuleManifest.h"
#include "KernelDiagnosticsLogger.h"
#include "KernelScreenReport.h"

OrynKernelModuleManifestItem* OrynKernelModuleManifestMutable(OrynKernelModuleId id);

static void SaveManifestStates(OrynKernelModuleState* states)
{
    for (unsigned int index = 0U; index < (unsigned int)OrynKernelModuleCount; ++index)
    {
        const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet((OrynKernelModuleId)index);
        states[index] = item ? item->State : OrynKernelModuleStateAbsent;
    }
}

static void RestoreManifestStates(const OrynKernelModuleState* states)
{
    for (unsigned int index = 0U; index < (unsigned int)OrynKernelModuleCount; ++index)
    {
        OrynKernelModuleManifestItem* item = OrynKernelModuleManifestMutable((OrynKernelModuleId)index);
        if (item)
        {
            item->State = states[index];
        }
    }
}

static int SetState(OrynKernelModuleId id, OrynKernelModuleState state)
{
    OrynKernelModuleManifestItem* item = OrynKernelModuleManifestMutable(id);
    if (!item)
    {
        return 0;
    }

    item->State = state;
    return 1;
}

static int SetPrerequisitesReady(OrynKernelModuleId id)
{
    unsigned int count = OrynKernelModuleManifestRequireCount(id);
    for (unsigned int index = 0U; index < count; ++index)
    {
        OrynKernelModuleId required = OrynKernelModuleManifestRequireAt(id, index);
        if (required >= OrynKernelModuleCount || !SetState(required, OrynKernelModuleStateReady))
        {
            return 0;
        }
    }

    return 1;
}

static int ProveSelectedStartingReady(void)
{
    if (!SetState(OrynKernelModuleApic, OrynKernelModuleStateRegistered))
    {
        return 0;
    }

    OrynKernelModuleManifestSelected(OrynKernelModuleApic);
    if (OrynKernelModuleManifestGet(OrynKernelModuleApic)->State != OrynKernelModuleStateSelected)
    {
        return 0;
    }

    if (!SetPrerequisitesReady(OrynKernelModuleApic))
    {
        return 0;
    }

    if (!OrynKernelModuleManifestBegin(OrynKernelModuleApic))
    {
        return 0;
    }

    if (OrynKernelModuleManifestGet(OrynKernelModuleApic)->State != OrynKernelModuleStateStarting)
    {
        return 0;
    }

    OrynKernelModuleManifestReady(OrynKernelModuleApic);
    return OrynKernelModuleManifestGet(OrynKernelModuleApic)->State == OrynKernelModuleStateReady;
}

static int ProveBlockedStartFails(void)
{
    if (!SetState(OrynKernelModuleApic, OrynKernelModuleStateSelected))
    {
        return 0;
    }

    (void)SetState(OrynKernelModuleCpu, OrynKernelModuleStateRegistered);
    (void)SetState(OrynKernelModuleInterrupts, OrynKernelModuleStateRegistered);
    (void)SetState(OrynKernelModulePic, OrynKernelModuleStateRegistered);

    if (OrynKernelModuleManifestBegin(OrynKernelModuleApic))
    {
        return 0;
    }

    return OrynKernelModuleManifestGet(OrynKernelModuleApic)->State == OrynKernelModuleStateFailed;
}

static int ProveSkippedOptional(void)
{
    if (!SetState(OrynKernelModuleHpet, OrynKernelModuleStateSelected))
    {
        return 0;
    }

    OrynKernelModuleManifestSkipped(OrynKernelModuleHpet);
    return OrynKernelModuleManifestGet(OrynKernelModuleHpet)->State == OrynKernelModuleStateSkipped;
}

void OrynKernelModuleManifestTransitionProof(void)
{
    OrynKernelModuleState states[OrynKernelModuleCount];
    SaveManifestStates(states);

    int readyProof = ProveSelectedStartingReady();
    RestoreManifestStates(states);

    int blockProof = ProveBlockedStartFails();
    RestoreManifestStates(states);

    int skipProof = ProveSkippedOptional();
    RestoreManifestStates(states);

    OrynKernelScreenReportOkOrFail(readyProof,
        "Module manifest proves selected -> starting -> ready transitions.",
        "Module manifest failed selected -> starting -> ready transition proof.");
    OrynKernelScreenReportOkOrFail(blockProof,
        "Module manifest proves blocked starts enter failed state.",
        "Module manifest failed blocked-start state proof.");
    OrynKernelScreenReportOkOrFail(skipProof,
        "Module manifest proves optional skipped state transition.",
        "Module manifest failed optional skipped state proof.");

    OrynKernelDiagnosticsLogText("[KERNEL] Module manifest transition proof: selected/start/ready, blocked/fail, optional/skipped.\n");
}
