#include "KernelIo.h"
#include "OrynBootInfo.h"
#include <stdio.h>

extern int KernelMain(const OrynBootInfo* bootInfo);

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
    int result;

    KernelIoInit();

    result = KernelMain(bootInfo);

#if !ORYN_VM_INTERACTIVE_DISPLAY
    if (result == 0)
    {
        KernelIoExitQemuSuccess();
    }
    else
    {
        printf("[KERNEL] FAIL: KernelMain returned failure code %d.\n", result);
        KernelIoExitQemuFailure();
    }
#endif

    OrynKernelEntryHaltLoop();
}
