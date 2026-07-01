#ifndef ORYN_KERNEL_INTERRUPT_LOCK_H
#define ORYN_KERNEL_INTERRUPT_LOCK_H

typedef struct OrynKernelInterruptLock
{
    volatile unsigned int Locked;
    unsigned int SavedInterruptState;
    unsigned int OwnerCpu;
    unsigned int Depth;
    unsigned long long AcquireCount;
    unsigned long long ReleaseCount;
    unsigned long long TryFailCount;
} OrynKernelInterruptLock;

typedef struct OrynKernelInterruptLockState
{
    unsigned int Initialized;
    unsigned int ProofRan;
    unsigned int ProofPassed;
    unsigned long long LocksInitialized;
    unsigned long long Acquires;
    unsigned long long Releases;
    unsigned long long TryFailures;
} OrynKernelInterruptLockState;

void OrynKernelInterruptLockInit(OrynKernelInterruptLock* lock);
void OrynKernelInterruptLockAcquire(OrynKernelInterruptLock* lock);
int OrynKernelInterruptLockTryAcquire(OrynKernelInterruptLock* lock);
void OrynKernelInterruptLockRelease(OrynKernelInterruptLock* lock);
int OrynKernelInterruptLockRunProof(void);
const OrynKernelInterruptLockState* OrynKernelInterruptLockGetState(void);
void OrynKernelInterruptLockPrintProof(void);

#endif
