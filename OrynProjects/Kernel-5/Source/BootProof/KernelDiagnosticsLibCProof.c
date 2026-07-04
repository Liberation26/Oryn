#include "KernelDiagnosticsProofsInternal.h"

void OrynKernelDiagnosticsRunLibCProof(void)
{
    if (!OrynKernelModuleManifestBegin(OrynKernelModuleLibC))
    {
        return;
    }

    int ok = OrynLibCRunSelfProof();
    OrynKernelScreenReportOkOrFail(ok,
        "OrynLibC freestanding libc proof complete.",
        "OrynLibC freestanding libc proof failed.");

    if (ok)
    {
        OrynKernelModuleManifestReady(OrynKernelModuleLibC);
    }
    else
    {
        OrynKernelModuleManifestFailed(OrynKernelModuleLibC);
    }
}
