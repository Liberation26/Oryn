#include "KernelDiagnosticsProofsInternal.h"
#include "KernelScreenReport.h"
#include "KernelUserExecutable.h"
#include "KernelModuleManifest.h"

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


static void OrynKernelDiagnosticsRunUserModeProof(void)
{
    OrynKernelProcess* process;
    OrynKernelUserProcess userProcess;
    OrynKernelThread* userThread;

    process = OrynKernelProcessCreate(&gPhysicalMemory, "ring3-proof", 0U);
    if (process == 0)
    {
        OrynKernelUserModePrintProof();
        return;
    }

    userProcess = OrynKernelUserProcessFromProcess(process);
    userThread = OrynKernelThreadCreateUser(
        &userProcess,
        "ring3-main",
        ORYN_USER_MODE_TEST_ENTRY,
        ORYN_USER_MODE_TEST_STACK);
    (void)OrynKernelUserModeRunProof(userThread);
    OrynKernelUserModePrintProof();
    OrynKernelThreadDestroy(userThread);
    OrynKernelProcessDestroy(process);
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
        if (OrynKernelPageFaultPolicyRunSelfTest())
        {
            gVirtualMemory.PageFaultPolicyReady = 1U;
        }
        if (OrynVirtualMemoryRunAddressSpaceSelfTest(&gVirtualMemory, &gPhysicalMemory))
        {
            OrynKernelScreenReportOk(0, "Virtual memory map/unmap/protect APIs passed proof.");
            OrynKernelScreenReportOk(0, "Per-process address-space creation passed proof.");
            OrynKernelScreenReportOk(0, "User/kernel address split is active.");
            OrynKernelScreenReportOk(0, "copy_from_user and copy_to_user safety helpers passed proof.");
            OrynKernelScreenReportOk(0, "Demand allocation for user anonymous pages passed proof.");

            /*
             * Process and Scheduler depend on VirtualMemory.  Mark the VM
             * module ready immediately after the VM proof succeeds, before
             * asking the manifest policy whether dependent modules may start.
             * Otherwise a valid Process selection is incorrectly reported as
             * missing the VirtualMemory prerequisite while VM is still in the
             * Starting state.
             */
            (void)OrynKernelLifecycleTransition(OrynKernelLifecycleVirtualMemoryReady);
            OrynKernelModuleManifestReady(OrynKernelModuleVirtualMemory);

            if (OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModuleProcess))
            {
                if (OrynKernelProcessRunSelfTest(&gPhysicalMemory))
                {
                    /*
                     * Scheduler depends on Process.  Mark Process ready as
                     * soon as its address-space and guarded kernel-thread
                     * proof succeeds, then evaluate Scheduler selection.
                     * This keeps manifest policy ordered by real state instead
                     * of checking both modules while Process is still starting.
                     */
                    OrynKernelModuleManifestReady(OrynKernelModuleProcess);
                    OrynKernelProcessPrintProof();
                    OrynKernelDiagnosticsRunUserModeProof();
                    if (OrynKernelModuleManifestIsReady(OrynKernelModuleVfs))
                    {
                        (void)OrynUserExecutableRunSelfTest(&gPhysicalMemory);
                        OrynUserExecutablePrintProof();
                    }

                    if (OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModuleScheduler))
                    {
                        const OrynKernelProcessStats* processStats = OrynKernelProcessGetStats();
                        if (processStats != 0 &&
                            processStats->SchedulerReadyThreadCount > 0U &&
                            processStats->KernelThreadStackCount > 0U)
                        {
                            OrynKernelModuleManifestReady(OrynKernelModuleScheduler);
                        }
                        else
                        {
                            OrynKernelModuleManifestFailed(OrynKernelModuleScheduler);
                            OrynKernelScreenReportFail(0, "Scheduler-ready stack proof failed.");
                        }
                    }
                }
                else
                {
                    OrynKernelProcessPrintProof();
                    OrynKernelModuleManifestFailed(OrynKernelModuleProcess);
                    OrynKernelScreenReportFail(0, "Process/thread scheduler stack proof failed.");
                    if (OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModuleScheduler))
                    {
                        OrynKernelModuleManifestFailed(OrynKernelModuleScheduler);
                    }
                }
            }
        }
        else
        {
            OrynKernelScreenReportFail(0, "Virtual memory address-space API proof failed.");
            OrynKernelModuleManifestFailed(OrynKernelModuleVirtualMemory);
        }
        OrynVirtualMemoryPrintProof(&gVirtualMemory);
        OrynKernelPageFaultPolicyPrintProof();
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
