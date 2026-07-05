#include "OrynBootInfo.h"
#include <stdio.h>

static inline void OrynKernelMainOut32(unsigned short port, unsigned int value)
{
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static void OrynKernelMainHaltLoop(void)
{
    for (;;)
    {
        __asm__ volatile ("cli");
        __asm__ volatile ("hlt");
    }
}

int KernelMain(const OrynBootInfo* bootInfo)
{
    int written;

    (void)bootInfo;

    written = printf("Hello World\n");
    if (written < 0)
    {
        return 1;
    }

    return 0;
}

void KernelStart(const OrynBootInfo* bootInfo)
{
    int result;

    result = KernelMain(bootInfo);

#if !ORYN_VM_INTERACTIVE_DISPLAY
    OrynKernelMainOut32(0xF4U, result == 0 ? 0x10U : 0x11U);
#endif

    OrynKernelMainHaltLoop();
}
