#include "KernelRuntime.h"

void KernelStart(const OrynBootInfo* bootInfo)
{
    OrynKernelRuntimeEnterAndStartBootSequence(bootInfo);
}
