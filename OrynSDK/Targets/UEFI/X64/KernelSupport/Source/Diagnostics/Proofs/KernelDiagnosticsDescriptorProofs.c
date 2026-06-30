#include "KernelDiagnosticsProofsInternal.h"

void OrynKernelDiagnosticsRunDescriptorProofs(void)
{
    (void)OrynKernelGdtInit();
    OrynKernelGdtPrintProof();
    (void)OrynKernelIdtInit();
    (void)OrynKernelInterruptsInit();
    OrynSysCallInit();
    (void)OrynKernelSysCallInterruptsInit();
    OrynKernelIdtPrintProof();
    OrynKernelInterruptsPrintProof();
    OrynSysCallPrintProof();
    OrynKernelSysCallInterruptsPrintProof();
    (void)OrynSysCallRunInternalProof();
    (void)OrynKernelSysCallInterruptsRunProof();
    OrynSysCallPrintRuntimeProof();
    OrynKernelSysCallInterruptsPrintRuntimeProof();
}
