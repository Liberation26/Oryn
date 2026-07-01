#include "KernelDiagnosticsProofsInternal.h"
#include "KernelScreenReport.h"

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

static void OrynKernelDiagnosticsRunHeapGuardProof(void)
{
    if (!OrynKernelModuleManifestIsReady(OrynKernelModuleHeap))
    {
        return;
    }

    OrynKernelHeapAttachVirtualMemory(&gVirtualMemory);
    OrynKernelHeapInstallStackGuard(gVirtualMemory.StackMapStart, gVirtualMemory.StackMapEnd - gVirtualMemory.StackMapStart);
    void* critical = OrynKernelHeapAllocCritical(96ULL);
    if (critical != 0)
    {
        kfree(critical);
    }
    OrynKernelHeapPrintProof();
}

static void OrynKernelDiagnosticsRunVirtualMemoryProof(const OrynBootInfo* kernelBootInfo)
{
    if (!OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModuleVirtualMemory))
    {
        return;
    }
    OrynKernelDiagnosticsLogText("[KERNEL] Virtual memory: starting\n");
    if (OrynVirtualMemoryInit(kernelBootInfo, &gKernelMemoryMap, &gPhysicalMemory, &gVirtualMemory))
    {
        if (OrynVirtualMemoryRunAddressSpaceSelfTest(&gVirtualMemory, &gPhysicalMemory))
        {
            OrynKernelScreenReportOk(0, "Virtual memory map/unmap/protect APIs passed proof.");
            OrynKernelScreenReportOk(0, "Per-process address-space creation passed proof.");
            OrynKernelScreenReportOk(0, "User/kernel address split is active.");
        }
        else
        {
            OrynKernelScreenReportFail(0, "Virtual memory address-space API proof failed.");
        }
        if (OrynKernelPageFaultPolicyRunSelfTest())
        {
            gVirtualMemory.PageFaultPolicyReady = 1U;
        }
        OrynVirtualMemoryPrintProof(&gVirtualMemory);
        OrynKernelPageFaultPolicyPrintProof();
        (void)OrynKernelLifecycleTransition(OrynKernelLifecycleVirtualMemoryReady);
        OrynKernelModuleManifestReady(OrynKernelModuleVirtualMemory);
        OrynKernelDiagnosticsRunHeapGuardProof();
    }
    else
    {
        OrynVirtualMemoryPrintProof(&gVirtualMemory);
        OrynKernelDiagnosticsLogText("[KERNEL] Virtual memory: failed\n");
        OrynKernelScreenReportWarn(0, "SMP AP startup skipped because virtual memory did not activate.");
        OrynKernelModuleManifestFailed(OrynKernelModuleVirtualMemory);
    }
}

static void OrynKernelDiagnosticsRunPhysicalMemoryProof(const OrynBootInfo* kernelBootInfo)
{
    if (!OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModulePhysicalMemory))
    {
        return;
    }
    if (OrynPhysicalMemoryInit(&gKernelMemoryMap, &gPhysicalMemory))
    {
        OrynKernelDiagnosticsReserveBootHandoffRanges(kernelBootInfo, &gPhysicalMemory);
        OrynPhysicalMemoryPrintSummary(&gPhysicalMemory);
        (void)OrynKernelLifecycleTransition(OrynKernelLifecycleMemoryReady);
        OrynPhysicalMemoryRunSelfTest(&gPhysicalMemory);
        OrynKernelModuleManifestReady(OrynKernelModulePhysicalMemory);

        if (OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModuleHeap))
        {
            if (OrynKernelHeapInit(&gPhysicalMemory) && OrynKernelHeapRunSelfTest())
            {
                OrynKernelScreenReportOk(0, "Kernel heap supports kmalloc, kfree, krealloc, and kcalloc.");
                OrynKernelScreenReportOk(0, "Kernel heap statistics and leak counters are active.");
                OrynKernelScreenReportOk(0, "Kernel slab/object caches are active for fixed-size structures.");
                OrynKernelModuleManifestReady(OrynKernelModuleHeap);
            }
            else
            {
                OrynKernelScreenReportFail(0, "Kernel heap proof failed.");
                OrynKernelModuleManifestFailed(OrynKernelModuleHeap);
            }
        }

        OrynKernelDiagnosticsRunVirtualMemoryProof(kernelBootInfo);
        OrynPhysicalMemoryPrintFinalState(&gPhysicalMemory);
    }
    else
    {
        OrynKernelDiagnosticsLogText("[KERNEL] Physical memory allocator: unavailable\n");
        OrynKernelModuleManifestFailed(OrynKernelModulePhysicalMemory);
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
        OrynKernelDiagnosticsLogText("[KERNEL] Memory map parser: unavailable.\n");
        OrynKernelDiagnosticsLogText("[KERNEL] Physical memory allocator: unavailable\n");
        OrynKernelModuleManifestFailed(OrynKernelModulePhysicalMemory);
    }
}
