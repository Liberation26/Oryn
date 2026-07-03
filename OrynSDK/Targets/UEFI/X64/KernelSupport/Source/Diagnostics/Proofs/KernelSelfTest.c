#include "KernelBootProof.h"
#include "KernelDiagnosticsLogger.h"
#include "KernelSelfTest.h"

void OrynKernelSelfTestRunCategoryChecks(const OrynBootInfo* kernelBootInfo)
{
    OrynKernelDiagnosticsLogText("[KERNEL] SelfTest owns boot proof category checks.\n");
    OrynKernelBootProofRunCategoryChecks(kernelBootInfo);
}
