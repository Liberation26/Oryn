#include "KernelDeferred.h"
#include "KernelInterruptLock.h"
#include "KernelInterrupts.h"
#include "KernelScreenReport.h"

static OrynKernelDeferredState gDeferredState;
static OrynKernelDeferredItem gQueue[ORYN_KERNEL_DEFERRED_LIMIT];
static OrynKernelInterruptLock gDeferredLock;
static unsigned int gHead;
static unsigned int gTail;
static unsigned int gProofCallbackCount;

static void ClearItem(OrynKernelDeferredItem* item)
{
    item->Used = 0U;
    item->Kind = OrynKernelDeferredKindNone;
    item->SourceVector = 0U;
    item->Routine = 0;
    item->Context = 0;
    item->Name = 0;
}

static void ProofCallback(void* context)
{
    unsigned int* value = (unsigned int*)context;
    if (value != 0)
    {
        *value += 1U;
    }
    gProofCallbackCount += 1U;
}

void OrynKernelDeferredInit(void)
{
    for (unsigned int index = 0U; index < ORYN_KERNEL_DEFERRED_LIMIT; ++index)
    {
        ClearItem(&gQueue[index]);
    }
    gHead = 0U;
    gTail = 0U;
    gDeferredState.Initialized = 1U;
    gDeferredState.Capacity = ORYN_KERNEL_DEFERRED_LIMIT;
    gDeferredState.Pending = 0U;
    gDeferredState.HighWatermark = 0U;
    OrynKernelInterruptLockInit(&gDeferredLock);
}

int OrynKernelDeferredQueue(unsigned int kind, unsigned int sourceVector,
    OrynKernelDeferredRoutine routine, void* context, const char* name)
{
    OrynKernelDeferredItem* item;
    if (gDeferredState.Initialized == 0U || routine == 0)
    {
        return 0;
    }
    OrynKernelInterruptLockAcquire(&gDeferredLock);
    if (gDeferredState.Pending >= ORYN_KERNEL_DEFERRED_LIMIT)
    {
        gDeferredState.Dropped += 1ULL;
        OrynKernelInterruptLockRelease(&gDeferredLock);
        return 0;
    }
    item = &gQueue[gTail];
    item->Used = 1U;
    item->Kind = kind;
    item->SourceVector = sourceVector;
    item->Routine = routine;
    item->Context = context;
    item->Name = name;
    gTail = (gTail + 1U) % ORYN_KERNEL_DEFERRED_LIMIT;
    gDeferredState.Pending += 1U;
    if (gDeferredState.Pending > gDeferredState.HighWatermark)
    {
        gDeferredState.HighWatermark = gDeferredState.Pending;
    }
    gDeferredState.Queued += 1ULL;
    if (OrynKernelInterruptsAreInInterrupt())
    {
        gDeferredState.InterruptQueued += 1ULL;
    }
    OrynKernelInterruptLockRelease(&gDeferredLock);
    return 1;
}

unsigned int OrynKernelDeferredRunPending(unsigned int budget)
{
    unsigned int ran = 0U;
    if (budget == 0U)
    {
        budget = ORYN_KERNEL_DEFERRED_LIMIT;
    }
    while (ran < budget)
    {
        OrynKernelDeferredItem item;
        OrynKernelInterruptLockAcquire(&gDeferredLock);
        if (gDeferredState.Pending == 0U)
        {
            OrynKernelInterruptLockRelease(&gDeferredLock);
            break;
        }
        item = gQueue[gHead];
        ClearItem(&gQueue[gHead]);
        gHead = (gHead + 1U) % ORYN_KERNEL_DEFERRED_LIMIT;
        gDeferredState.Pending -= 1U;
        OrynKernelInterruptLockRelease(&gDeferredLock);
        if (item.Used && item.Routine != 0)
        {
            item.Routine(item.Context);
            gDeferredState.Executed += 1ULL;
            ran += 1U;
        }
    }
    return ran;
}

int OrynKernelDeferredRunProof(void)
{
    unsigned int proofValue = 0U;
    gDeferredState.ProofRan = 1U;
    OrynKernelDeferredInit();
    if (!OrynKernelDeferredQueue(OrynKernelDeferredKindBottomHalf, 0xEEU,
        ProofCallback, &proofValue, "proof-bottom-half"))
    {
        return 0;
    }
    if (!OrynKernelDeferredQueue(OrynKernelDeferredKindDpc, 0xEFU,
        ProofCallback, &proofValue, "proof-dpc"))
    {
        return 0;
    }
    if (OrynKernelDeferredRunPending(8U) != 2U)
    {
        return 0;
    }
    gDeferredState.ProofPassed =
        (proofValue == 2U && gProofCallbackCount >= 2U && gDeferredState.Executed >= 2ULL) ? 1U : 0U;
    return gDeferredState.ProofPassed ? 1 : 0;
}

const OrynKernelDeferredState* OrynKernelDeferredGetState(void)
{
    return &gDeferredState;
}

void OrynKernelDeferredPrintProof(void)
{
    OrynKernelScreenReportOkOrFail(gDeferredState.ProofPassed,
        "Bottom halves/deferred procedure calls queue interrupt follow-up work.",
        "Bottom half/deferred procedure call proof failed.");
}
