#include "KernelDiagnosticsProofsInternal.h"

int OrynKernelDiagnosticsRunKeyboardScrollProof(void)
{
    int ok = OrynKernelKeyboardInitForConsoleScroll();
    OrynKernelKeyboardPrintProof();
    return ok;
}
