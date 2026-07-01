#include "KernelDiagnosticsProofsInternal.h"

static void StartGdt(void)
{
    if (!OrynKernelDiagnosticsShouldStartModule(0, OrynKernelModuleGdt))
    {
        return;
    }
    if (OrynKernelGdtInit())
    {
        OrynKernelModuleManifestReady(OrynKernelModuleGdt);
    }
    else
    {
        OrynKernelModuleManifestFailed(OrynKernelModuleGdt);
    }
    OrynKernelGdtPrintProof();
}

static void StartIdt(void)
{
    if (!OrynKernelDiagnosticsShouldStartModule(0, OrynKernelModuleIdt))
    {
        return;
    }
    if (OrynKernelIdtInit())
    {
        OrynKernelModuleManifestReady(OrynKernelModuleIdt);
    }
    else
    {
        OrynKernelModuleManifestFailed(OrynKernelModuleIdt);
    }
    OrynKernelIdtPrintProof();
}

static void StartInterrupts(void)
{
    if (!OrynKernelDiagnosticsShouldStartModule(0, OrynKernelModuleInterrupts))
    {
        return;
    }
    if (OrynKernelInterruptsInit())
    {
        OrynKernelModuleManifestReady(OrynKernelModuleInterrupts);
    }
    else
    {
        OrynKernelModuleManifestFailed(OrynKernelModuleInterrupts);
    }
    OrynKernelInterruptsPrintProof();
}

static void StartSysCalls(void)
{
    if (!OrynKernelDiagnosticsShouldStartModule(0, OrynKernelModuleSysCalls))
    {
        return;
    }
    OrynSysCallInit();
    (void)OrynKernelSysCallInterruptsInit();
    OrynSysCallPrintProof();
    OrynKernelSysCallInterruptsPrintProof();
    (void)OrynSysCallRunInternalProof();
    (void)OrynKernelSysCallInterruptsRunProof();
    OrynSysCallPrintRuntimeProof();
    OrynKernelSysCallInterruptsPrintRuntimeProof();
    OrynKernelModuleManifestReady(OrynKernelModuleSysCalls);
}

void OrynKernelDiagnosticsRunDescriptorProofs(void)
{
    StartGdt();
    StartIdt();
    StartInterrupts();
    StartSysCalls();
}
