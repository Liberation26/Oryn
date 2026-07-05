#include "KernelIo.h"
#include "KernelRuntimeStartup.h"
#include "OrynBootInfo.h"
#include <stdbool.h>
#include <stdio.h>

extern bool KernelMain(const OrynBootInfo* bootInfo);

static void OrynKernelEntryHaltLoop(void)
{
    for (;;)
    {
        __asm__ volatile ("cli" ::: "memory");
        __asm__ volatile ("hlt");
    }
}

void KernelStart(const OrynBootInfo* bootInfo)
{
    bool succeeded;

    KernelIoInit();

    OrynKernelRuntimeStartupResult startup = OrynKernelRuntimeStartSelectedModules(bootInfo);
    if (!OrynKernelRuntimeStartupSucceeded(&startup))
    {
#if !ORYN_VM_INTERACTIVE_DISPLAY
        KernelIoExitQemuFailure();
#endif
        OrynKernelEntryHaltLoop();
    }

    succeeded = KernelMain(bootInfo);

#if !ORYN_VM_INTERACTIVE_DISPLAY
    if (succeeded)
    {
        KernelIoExitQemuSuccess();
    }
    else
    {
        printf("[KERNEL] FAIL: KernelMain returned false.\n");
        KernelIoExitQemuFailure();
    }
#endif

    OrynKernelEntryHaltLoop();
}
