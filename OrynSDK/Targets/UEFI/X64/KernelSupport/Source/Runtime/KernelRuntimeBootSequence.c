#include "KernelBootProof.h"
#include "KernelRuntime.h"
#include "KernelRuntimeInternal.h"

void OrynKernelRuntimeEnterAndStartBootSequence(const OrynBootInfo* bootInfo)
{
    const OrynBootInfo* kernelBootInfo = OrynKernelRuntimeEnter(bootInfo);
    OrynKernelBootProofRunSequence(kernelBootInfo);
    OrynKernelRuntimeExitForNonInteractiveVm();
    OrynKernelRuntimeHaltForever();
}
