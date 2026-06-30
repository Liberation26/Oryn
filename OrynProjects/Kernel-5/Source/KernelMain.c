#include "Kernel.h"
#include "KernelRuntime.h"

void KernelStart(const OrynBootInfo* bootInfo)
{
    OrynKernelRuntimeStartBootSequence(bootInfo);
}
