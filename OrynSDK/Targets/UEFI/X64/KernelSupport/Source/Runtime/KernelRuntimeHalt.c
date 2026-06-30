#include "KernelIo.h"
#include "KernelRuntimeInternal.h"

#ifndef ORYN_VM_INTERACTIVE_DISPLAY
#define ORYN_VM_INTERACTIVE_DISPLAY 0
#endif

void OrynKernelRuntimeExitForNonInteractiveVm(void)
{
#if !ORYN_VM_INTERACTIVE_DISPLAY
    KernelIoExitQemuSuccess();
#endif
}

void OrynKernelRuntimeHaltForever(void)
{
#if ORYN_VM_INTERACTIVE_DISPLAY
    for (;;)
    {
        __asm__ volatile ("sti");
        __asm__ volatile ("hlt");
    }
#else
    for (;;)
    {
        __asm__ volatile ("cli");
        __asm__ volatile ("hlt");
    }
#endif
}
