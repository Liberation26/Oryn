#ifndef ORYN_KERNEL_DEFERRED_H
#define ORYN_KERNEL_DEFERRED_H

#define ORYN_KERNEL_DEFERRED_LIMIT 128U

typedef void (*OrynKernelDeferredRoutine)(void* context);

typedef enum OrynKernelDeferredKind
{
    OrynKernelDeferredKindNone = 0,
    OrynKernelDeferredKindBottomHalf = 1,
    OrynKernelDeferredKindDpc = 2
} OrynKernelDeferredKind;

typedef struct OrynKernelDeferredItem
{
    unsigned int Used;
    unsigned int Kind;
    unsigned int SourceVector;
    OrynKernelDeferredRoutine Routine;
    void* Context;
    const char* Name;
} OrynKernelDeferredItem;

typedef struct OrynKernelDeferredState
{
    unsigned int Initialized;
    unsigned int Capacity;
    unsigned int Pending;
    unsigned int HighWatermark;
    unsigned int ProofRan;
    unsigned int ProofPassed;
    unsigned long long Queued;
    unsigned long long Executed;
    unsigned long long Dropped;
    unsigned long long InterruptQueued;
} OrynKernelDeferredState;

void OrynKernelDeferredInit(void);
int OrynKernelDeferredQueue(unsigned int kind, unsigned int sourceVector,
    OrynKernelDeferredRoutine routine, void* context, const char* name);
unsigned int OrynKernelDeferredRunPending(unsigned int budget);
int OrynKernelDeferredRunProof(void);
const OrynKernelDeferredState* OrynKernelDeferredGetState(void);
void OrynKernelDeferredPrintProof(void);

#endif
