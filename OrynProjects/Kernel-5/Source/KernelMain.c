#include "OrynBootInfo.h"
#include <stdio.h>

#define ORYN_KERNEL_QEMU_EXIT_PORT 0xF4U
#define ORYN_KERNEL_QEMU_EXIT_SUCCESS 0x10U
#define ORYN_KERNEL_QEMU_EXIT_FAILURE 0x11U

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
    if (result == 0)
    {
        printf("[KERNEL] Requesting QEMU debug-exit success.\n");
        OrynKernelMainOut32(ORYN_KERNEL_QEMU_EXIT_PORT, ORYN_KERNEL_QEMU_EXIT_SUCCESS);
    }
    else
    {
        printf("[KERNEL] FAIL: KernelMain returned failure.\n");
        OrynKernelMainOut32(ORYN_KERNEL_QEMU_EXIT_PORT, ORYN_KERNEL_QEMU_EXIT_FAILURE);
    }
#endif

    OrynKernelMainHaltLoop();
}
