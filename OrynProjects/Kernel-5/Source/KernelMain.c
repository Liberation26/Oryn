#include "Kernel.h"
#include "KernelBootInfo.h"
#include "KernelConsole.h"
#include "KernelIo.h"
#include "KernelMemoryMap.h"
#include "KernelPhysicalMemory.h"
#include "KernelVirtualMemory.h"

static OrynKernelMemoryMap gKernelMemoryMap;
static OrynKernelPhysicalMemory gPhysicalMemory;
static OrynKernelVirtualMemory gVirtualMemory;

void KernelStart(const OrynBootInfo* bootInfo)
{
    KernelIoInit();
    KConsoleInit(bootInfo);
    KConsole.ClearScreen();
    KernelIoWriteString("[KERNEL] Oryn Kernel-5 booted.\n");
    KernelIoWriteString("[KERNEL] Target: uefi-x64\n");
    KernelIoWriteString("[KERNEL] Toolchain: clang + lld\n");
    KernelIoWriteString("[KERNEL] PASS: Kernel entered successfully.\n");
    KernelIoWriteString("[KERNEL] PASS: Serial/debug output path is working.\n");
    KernelIoWriteString(KConsole.IsTtfActive() ? "[KERNEL] TTF renderer: active\n" : "[KERNEL] TTF renderer: fallback bitmap glyphs\n");

    KernelBootInfoPrintSelection();
    OrynKernelBootInfoStatus bootStatus = KernelBootInfoValidate(bootInfo);
    if (bootStatus.IsValid)
    {
        KernelBootInfoPrintSummary(bootInfo);
        if (OrynMemoryMapBuildFromBootInfo(bootInfo, &gKernelMemoryMap))
        {
            OrynMemoryMapPrintSummary(&gKernelMemoryMap);
            if (OrynPhysicalMemoryInit(&gKernelMemoryMap, &gPhysicalMemory))
            {
                OrynPhysicalMemoryPrintSummary(&gPhysicalMemory);
                OrynPhysicalMemoryRunSelfTest(&gPhysicalMemory);
                KernelIoWriteString("[KERNEL] Virtual memory: starting\n");
                if (OrynVirtualMemoryInit(bootInfo, &gKernelMemoryMap, &gPhysicalMemory, &gVirtualMemory))
                {
                    OrynVirtualMemoryPrintProof(&gVirtualMemory);
                }
                else
                {
                    OrynVirtualMemoryPrintProof(&gVirtualMemory);
                    KernelIoWriteString("[KERNEL] Virtual memory: failed\n");
                }
                OrynPhysicalMemoryPrintFinalState(&gPhysicalMemory);
            }
            else
            {
                KernelIoWriteString("[KERNEL] Physical memory allocator: unavailable\n");
            }
        }
        else
        {
            KernelIoWriteString("[KERNEL] Memory map parser: unavailable.\n");
            KernelIoWriteString("[KERNEL] Physical memory allocator: unavailable\n");
        }
    }
    else
    {
        KernelIoWriteString("[KERNEL] BootInfo invalid. Memory services are disabled.\n");
    }

    KernelIoWriteString("[KERNEL] System halted by Kernel-5.\n");
    KernelIoExitQemuSuccess();

    for (;;)
    {
        __asm__ volatile ("cli");
        __asm__ volatile ("hlt");
    }
}
