#include "KernelDiagnosticsProofsInternal.h"

static OrynKernelMemoryMap gKernelMemoryMap;
static OrynKernelPhysicalMemory gPhysicalMemory;
static OrynKernelVirtualMemory gVirtualMemory;

static void OrynKernelDiagnosticsReserveRangeIfPresent(
    OrynKernelPhysicalMemory* physicalMemory,
    unsigned long long start,
    unsigned long long bytes)
{
    if (start != 0ULL && bytes != 0ULL)
    {
        (void)OrynPhysicalMemoryReserveRange(physicalMemory, start, bytes);
    }
}

static void OrynKernelDiagnosticsReserveBootHandoffRanges(
    const OrynBootInfo* bootInfo,
    OrynKernelPhysicalMemory* physicalMemory)
{
    if (bootInfo == 0 || physicalMemory == 0)
    {
        return;
    }

    OrynKernelDiagnosticsReserveRangeIfPresent(physicalMemory, (unsigned long long)bootInfo, sizeof(*bootInfo));
    OrynKernelDiagnosticsReserveRangeIfPresent(physicalMemory, KernelBootInfoSourceAddress(), sizeof(*bootInfo));

    if (KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_KERNEL_RANGE))
    {
        OrynKernelDiagnosticsReserveRangeIfPresent(physicalMemory, bootInfo->KernelPhysicalBase, bootInfo->KernelSize);
    }

    if (KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_MEMORY_MAP) &&
        bootInfo->MemoryMapEntrySize != 0ULL)
    {
        OrynKernelDiagnosticsReserveRangeIfPresent(
            physicalMemory,
            bootInfo->MemoryMap,
            bootInfo->MemoryMapEntryCount * bootInfo->MemoryMapEntrySize);
    }

    if (KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_FONT))
    {
        OrynKernelDiagnosticsReserveRangeIfPresent(physicalMemory, bootInfo->FontBase, bootInfo->FontSize);
    }

    if (KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_FRAMEBUFFER))
    {
        OrynKernelDiagnosticsReserveRangeIfPresent(
            physicalMemory,
            bootInfo->Framebuffer.Base,
            bootInfo->Framebuffer.Size);
    }

    OrynKernelDiagnosticsReserveRangeIfPresent(physicalMemory, ORYN_SMP_TRAMPOLINE_BASE, 4096ULL);
}

static void OrynKernelDiagnosticsRunVirtualMemoryProof(const OrynBootInfo* kernelBootInfo)
{
    KernelIoWriteString("[KERNEL] Virtual memory: starting\n");
    if (OrynVirtualMemoryInit(kernelBootInfo, &gKernelMemoryMap, &gPhysicalMemory, &gVirtualMemory))
    {
        OrynVirtualMemoryPrintProof(&gVirtualMemory);
        (void)OrynKernelLifecycleTransition(OrynKernelLifecycleVirtualMemoryReady);
    }
    else
    {
        OrynVirtualMemoryPrintProof(&gVirtualMemory);
        KernelIoWriteString("[KERNEL] Virtual memory: failed\n");
        KernelIoWriteString("[KERNEL] WARN: SMP AP startup skipped because virtual memory did not activate.\n");
    }
}

static void OrynKernelDiagnosticsRunPhysicalMemoryProof(const OrynBootInfo* kernelBootInfo)
{
    if (OrynPhysicalMemoryInit(&gKernelMemoryMap, &gPhysicalMemory))
    {
        OrynKernelDiagnosticsReserveBootHandoffRanges(kernelBootInfo, &gPhysicalMemory);
        OrynPhysicalMemoryPrintSummary(&gPhysicalMemory);
        (void)OrynKernelLifecycleTransition(OrynKernelLifecycleMemoryReady);
        OrynPhysicalMemoryRunSelfTest(&gPhysicalMemory);
        OrynKernelDiagnosticsRunVirtualMemoryProof(kernelBootInfo);
        OrynPhysicalMemoryPrintFinalState(&gPhysicalMemory);
    }
    else
    {
        KernelIoWriteString("[KERNEL] Physical memory allocator: unavailable\n");
    }
}

void OrynKernelDiagnosticsRunMemoryProofs(const OrynBootInfo* kernelBootInfo)
{
    if (OrynMemoryMapBuildFromBootInfo(kernelBootInfo, &gKernelMemoryMap))
    {
        OrynMemoryMapPrintSummary(&gKernelMemoryMap);
        OrynKernelDiagnosticsRunPhysicalMemoryProof(kernelBootInfo);
    }
    else
    {
        KernelIoWriteString("[KERNEL] Memory map parser: unavailable.\n");
        KernelIoWriteString("[KERNEL] Physical memory allocator: unavailable\n");
    }
}
