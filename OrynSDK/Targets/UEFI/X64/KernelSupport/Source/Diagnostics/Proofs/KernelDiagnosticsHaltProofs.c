#include "KernelDiagnostics.h"
#include "KernelDiagnosticsProofsInternal.h"
#include "KernelScreenReport.h"

#if ORYN_VM_INTERACTIVE_DISPLAY
static void OrynKernelDiagnosticsRunInteractiveHaltProofs(void)
{
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleInteractiveHalt);
    OrynKernelKeyboardEnableInteractiveInterrupts();
    int keyboardInterruptsReady = OrynKernelInterruptsAreEnabled() ? 1 : 0;
    OrynKernelInterruptsDisable();
    OrynKernelScreenReportOkOrFail(keyboardInterruptsReady,
        "Interactive halt loop leaves interrupts enabled for keyboard scrolling.",
        "Interactive halt loop could not enable keyboard interrupts.");
    OrynKernelScreenReportOk(0, "Interactive QEMU display mode keeps VM open for scroll testing.");
    KernelIoWriteString("[KERNEL] INFO: Use Up/Down to scroll one line and PgUp/PgDn to scroll one page.\n");
    KernelIoWriteString("[KERNEL] INFO: Close the QEMU window after manual scroll testing is complete.\n");
    if (keyboardInterruptsReady)
    {
        OrynKernelInterruptsEnable();
    }
}
#endif

void OrynKernelDiagnosticsRunHaltProofs(void)
{
#if ORYN_VM_INTERACTIVE_DISPLAY
    OrynKernelDiagnosticsRunInteractiveHaltProofs();
#else
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleDebugExitRequested);
#endif

    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleHalting);
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleHalted);
    OrynKernelLifecyclePrintProof();
    OrynKernelScreenReportPrint();
}
