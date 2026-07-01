#include "KernelInterruptLock.h"
#include "KernelInterrupts.h"
#include "KernelScreenReport.h"

static OrynKernelInterruptLockState gInterruptLockState;

static unsigned int AtomicExchange(volatile unsigned int* target, unsigned int value)
{
    return __atomic_exchange_n(target, value, __ATOMIC_ACQUIRE);
}

static void AtomicStore(volatile unsigned int* target, unsigned int value)
{
    __atomic_store_n(target, value, __ATOMIC_RELEASE);
}

void OrynKernelInterruptLockInit(OrynKernelInterruptLock* lock)
{
    if (lock == 0)
    {
        return;
    }
    lock->Locked = 0U;
    lock->SavedInterruptState = 0U;
    lock->OwnerCpu = 0U;
    lock->Depth = 0U;
    lock->AcquireCount = 0ULL;
    lock->ReleaseCount = 0ULL;
    lock->TryFailCount = 0ULL;
    gInterruptLockState.Initialized = 1U;
    gInterruptLockState.LocksInitialized += 1ULL;
}

void OrynKernelInterruptLockAcquire(OrynKernelInterruptLock* lock)
{
    unsigned int wasEnabled;
    if (lock == 0)
    {
        return;
    }
    wasEnabled = OrynKernelInterruptsAreEnabled();
    OrynKernelInterruptsDisable();
    while (AtomicExchange(&lock->Locked, 1U) != 0U)
    {
        __asm__ volatile ("pause");
    }
    lock->SavedInterruptState = wasEnabled;
    lock->OwnerCpu = OrynKernelInterruptsGetCurrentCpuIndex();
    lock->Depth = 1U;
    lock->AcquireCount += 1ULL;
    gInterruptLockState.Acquires += 1ULL;
}

int OrynKernelInterruptLockTryAcquire(OrynKernelInterruptLock* lock)
{
    unsigned int wasEnabled;
    if (lock == 0)
    {
        return 0;
    }
    wasEnabled = OrynKernelInterruptsAreEnabled();
    OrynKernelInterruptsDisable();
    if (AtomicExchange(&lock->Locked, 1U) != 0U)
    {
        lock->TryFailCount += 1ULL;
        gInterruptLockState.TryFailures += 1ULL;
        if (wasEnabled)
        {
            OrynKernelInterruptsEnable();
        }
        return 0;
    }
    lock->SavedInterruptState = wasEnabled;
    lock->OwnerCpu = OrynKernelInterruptsGetCurrentCpuIndex();
    lock->Depth = 1U;
    lock->AcquireCount += 1ULL;
    gInterruptLockState.Acquires += 1ULL;
    return 1;
}

void OrynKernelInterruptLockRelease(OrynKernelInterruptLock* lock)
{
    unsigned int restore;
    if (lock == 0 || lock->Locked == 0U)
    {
        return;
    }
    restore = lock->SavedInterruptState;
    lock->Depth = 0U;
    lock->ReleaseCount += 1ULL;
    gInterruptLockState.Releases += 1ULL;
    AtomicStore(&lock->Locked, 0U);
    if (restore)
    {
        OrynKernelInterruptsEnable();
    }
}

int OrynKernelInterruptLockRunProof(void)
{
    OrynKernelInterruptLock lock;
    gInterruptLockState.ProofRan = 1U;
    OrynKernelInterruptLockInit(&lock);
    OrynKernelInterruptLockAcquire(&lock);
    if (lock.Locked == 0U || lock.AcquireCount == 0ULL)
    {
        return 0;
    }
    if (OrynKernelInterruptLockTryAcquire(&lock) != 0)
    {
        return 0;
    }
    OrynKernelInterruptLockRelease(&lock);
    gInterruptLockState.ProofPassed =
        (lock.Locked == 0U && lock.ReleaseCount == 1ULL && lock.TryFailCount == 1ULL) ? 1U : 0U;
    return gInterruptLockState.ProofPassed ? 1 : 0;
}

const OrynKernelInterruptLockState* OrynKernelInterruptLockGetState(void)
{
    return &gInterruptLockState;
}

void OrynKernelInterruptLockPrintProof(void)
{
    OrynKernelScreenReportOkOrFail(gInterruptLockState.ProofPassed,
        "Interrupt-safe lock primitives disable interrupts and serialize access.",
        "Interrupt-safe lock primitive proof failed.");
}
