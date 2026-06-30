#include "KernelDiagnosticsProofsInternal.h"

void OrynKernelDiagnosticsRunPciProof(const OrynBootInfo* kernelBootInfo)
{
    OrynKernelPciInit(kernelBootInfo);
    OrynKernelPciPrintProof();
}
