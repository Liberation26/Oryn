#include "Kernel.h"
#include "KernelBootInfo.h"
#include "KernelConsole.h"
#include "KernelIo.h"
#include "KernelCpu.h"
#include "KernelGdt.h"
#include "KernelIdt.h"
#include "KernelInterrupts.h"
#include "KernelPic.h"
#include "KernelApic.h"
#include "KernelHpet.h"
#include "KernelMemoryMap.h"
#include "KernelPhysicalMemory.h"
#include "KernelVirtualMemory.h"

static OrynKernelMemoryMap gKernelMemoryMap;
static OrynKernelPhysicalMemory gPhysicalMemory;
static OrynKernelVirtualMemory gVirtualMemory;

static void KernelDisableInterrupts(void)
{
    __asm__ volatile ("cli" ::: "memory");
}

static void ReserveRangeIfPresent(
    OrynKernelPhysicalMemory* physicalMemory,
    unsigned long long start,
    unsigned long long bytes)
{
    if (start != 0ULL && bytes != 0ULL)
    {
        (void)OrynPhysicalMemoryReserveRange(physicalMemory, start, bytes);
    }
}

static void ReserveBootHandoffRanges(
    const OrynBootInfo* bootInfo,
    OrynKernelPhysicalMemory* physicalMemory)
{
    if (bootInfo == 0 || physicalMemory == 0)
    {
        return;
    }

    ReserveRangeIfPresent(physicalMemory, (unsigned long long)bootInfo, sizeof(*bootInfo));
    ReserveRangeIfPresent(physicalMemory, KernelBootInfoSourceAddress(), sizeof(*bootInfo));

    if (KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_KERNEL_RANGE))
    {
        ReserveRangeIfPresent(physicalMemory, bootInfo->KernelPhysicalBase, bootInfo->KernelSize);
    }

    if (KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_MEMORY_MAP) &&
        bootInfo->MemoryMapEntrySize != 0ULL)
    {
        ReserveRangeIfPresent(
            physicalMemory,
            bootInfo->MemoryMap,
            bootInfo->MemoryMapEntryCount * bootInfo->MemoryMapEntrySize);
    }

    if (KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_FONT))
    {
        ReserveRangeIfPresent(physicalMemory, bootInfo->FontBase, bootInfo->FontSize);
    }

    if (KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_FRAMEBUFFER))
    {
        ReserveRangeIfPresent(
            physicalMemory,
            bootInfo->Framebuffer.Base,
            bootInfo->Framebuffer.Size);
    }
}

void KernelStart(const OrynBootInfo* bootInfo)
{
    KernelDisableInterrupts();
    const OrynBootInfo* kernelBootInfo = KernelBootInfoAdopt(bootInfo);

    KernelIoInit();
    KernelIoWriteString("[KERNEL] Oryn Kernel-5 entered.\n");
    KernelIoWriteString("[KERNEL] PASS: Serial/debug output path is working.\n");
    (void)OrynKernelGdtInit();
    OrynKernelGdtPrintProof();
    (void)OrynKernelIdtInit();
    (void)OrynKernelInterruptsInit();
    OrynKernelIdtPrintProof();
    OrynKernelInterruptsPrintProof();
    OrynKernelCpuDetect();
    OrynKernelCpuPrintFeatures();
    (void)OrynKernelPicInitAndDisable();
    OrynKernelPicPrintProof();
    (void)OrynKernelInterruptsRunPicTimerProof();
    (void)OrynKernelApicInit(1);
    OrynKernelApicPrintProof();
    (void)OrynKernelHpetInit(kernelBootInfo);
    OrynKernelHpetPrintProof();
    (void)OrynKernelInterruptsRunApicTimerProof();
    OrynKernelInterruptsPrintRuntimeProof();
    KernelIoWriteString("[KERNEL] PASS: Kernel entry received one plain OrynBootInfo pointer.\n");
    if (KernelBootInfoIsKernelOwned(kernelBootInfo))
    {
        KernelIoWriteString("[KERNEL] PASS: OrynBootInfo copied into kernel-owned storage.\n");
        KernelIoWriteString("[KERNEL] Loader BootInfo pointer: ");
        KernelIoWriteHex64(KernelBootInfoSourceAddress());
        KernelIoWriteString("\n");
        KernelIoWriteString("[KERNEL] Kernel BootInfo copy: ");
        KernelIoWriteHex64((unsigned long long)kernelBootInfo);
        KernelIoWriteString("\n");
    }
    else
    {
        KernelIoWriteString("[KERNEL] FAIL: Kernel could not adopt OrynBootInfo.\n");
    }

    KernelBootInfoPrintSelection();
    OrynKernelBootInfoStatus bootStatus = KernelBootInfoValidate(kernelBootInfo);
    if (bootStatus.IsValid)
    {
        KConsoleInit(kernelBootInfo);
        KConsole.ClearScreen();
        KernelIoWriteString("[KERNEL] Oryn Kernel-5 booted.\n");
        KernelIoWriteString("[KERNEL] Target: uefi-x64\n");
        KernelIoWriteString("[KERNEL] Toolchain: clang + lld\n");
        KernelIoWriteString("[KERNEL] PASS: Kernel entered successfully.\n");
        KernelIoWriteString(KConsole.IsTtfActive() ? "[KERNEL] TTF renderer: active\n" : "[KERNEL] TTF renderer: fallback bitmap glyphs\n");
        KernelBootInfoPrintSummary(kernelBootInfo);
        if (OrynMemoryMapBuildFromBootInfo(kernelBootInfo, &gKernelMemoryMap))
        {
            OrynMemoryMapPrintSummary(&gKernelMemoryMap);
            if (OrynPhysicalMemoryInit(&gKernelMemoryMap, &gPhysicalMemory))
            {
                ReserveBootHandoffRanges(kernelBootInfo, &gPhysicalMemory);
                OrynPhysicalMemoryPrintSummary(&gPhysicalMemory);
                OrynPhysicalMemoryRunSelfTest(&gPhysicalMemory);
                KernelIoWriteString("[KERNEL] Virtual memory: starting\n");
                if (OrynVirtualMemoryInit(kernelBootInfo, &gKernelMemoryMap, &gPhysicalMemory, &gVirtualMemory))
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
