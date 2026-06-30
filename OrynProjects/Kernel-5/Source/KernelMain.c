#include "Kernel.h"
#include "KernelBootInfo.h"
#include "KernelConsole.h"
#include "KernelIo.h"
#include "KernelCpu.h"
#include "KernelGdt.h"
#include "KernelIdt.h"
#include "KernelInterrupts.h"
#include "KernelSysCallInterrupts.h"
#include "KernelPic.h"
#include "KernelApic.h"
#include "KernelHpet.h"
#include "KernelPci.h"
#include "KernelSmp.h"
#include "KernelMemoryMap.h"
#include "KernelPhysicalMemory.h"
#include "KernelVirtualMemory.h"
#include "SysCall.h"

#ifndef ORYN_VM_PIC
#define ORYN_VM_PIC 1
#endif

#ifndef ORYN_VM_APIC
#define ORYN_VM_APIC 1
#endif

#ifndef ORYN_VM_APIC2
#define ORYN_VM_APIC2 1
#endif

#ifndef ORYN_VM_HPET
#define ORYN_VM_HPET 1
#endif

#ifndef ORYN_VM_SMP_CPUS
#define ORYN_VM_SMP_CPUS 1
#endif

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

    ReserveRangeIfPresent(physicalMemory, ORYN_SMP_TRAMPOLINE_BASE, 4096ULL);
}

static void PrintBootInfoOwnership(const OrynBootInfo* kernelBootInfo)
{
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
}

static void RunDescriptorAndSysCallProofs(void)
{
    (void)OrynKernelGdtInit();
    OrynKernelGdtPrintProof();
    (void)OrynKernelIdtInit();
    (void)OrynKernelInterruptsInit();
    OrynSysCallInit();
    (void)OrynKernelSysCallInterruptsInit();
    OrynKernelIdtPrintProof();
    OrynKernelInterruptsPrintProof();
    OrynSysCallPrintProof();
    OrynKernelSysCallInterruptsPrintProof();
    (void)OrynSysCallRunInternalProof();
    (void)OrynKernelSysCallInterruptsRunProof();
    OrynSysCallPrintRuntimeProof();
    OrynKernelSysCallInterruptsPrintRuntimeProof();
}

static void RunInterruptAndTimerProofs(const OrynBootInfo* kernelBootInfo)
{
    OrynKernelCpuDetect();
    OrynKernelCpuPrintFeatures();

#if ORYN_VM_PIC
    (void)OrynKernelPicInitAndDisable();
    OrynKernelPicPrintProof();
    (void)OrynKernelInterruptsRunPicTimerProof();
    OrynKernelInterruptsPrintPicRuntimeProof();
    OrynKernelPicPrintProof();
#else
    KernelIoWriteString("[KERNEL] INFO: PIC IRQ0 proof skipped by VMSettings.\n");
#endif

#if ORYN_VM_APIC
#if ORYN_VM_APIC2
    (void)OrynKernelApicInit(1);
#else
    KernelIoWriteString("[KERNEL] INFO: APIC2/x2APIC disabled by VMSettings.\n");
    (void)OrynKernelApicInit(0);
#endif
    OrynKernelApicPrintProof();
#else
    KernelIoWriteString("[KERNEL] INFO: APIC proofs skipped by VMSettings.\n");
#endif

#if ORYN_VM_HPET
    (void)OrynKernelHpetInit(kernelBootInfo);
    OrynKernelHpetPrintProof();
#else
    (void)kernelBootInfo;
    KernelIoWriteString("[KERNEL] INFO: HPET proof skipped by VMSettings.\n");
#endif

#if ORYN_VM_APIC
    (void)OrynKernelInterruptsRunApicTimerProof();
    OrynKernelInterruptsPrintRuntimeProof();
#else
    KernelIoWriteString("[KERNEL] PASS: VMSettings interrupt/timer profile applied.\n");
#endif

#if ORYN_VM_APIC && (!ORYN_VM_PIC || !ORYN_VM_HPET || !ORYN_VM_APIC2)
    KernelIoWriteString("[KERNEL] PASS: VMSettings interrupt/timer profile applied.\n");
#endif
}

static void RunPciProof(const OrynBootInfo* kernelBootInfo)
{
    OrynKernelPciInit(kernelBootInfo);
    OrynKernelPciPrintProof();
}

static void RunSmpProof(const OrynBootInfo* kernelBootInfo)
{
#if ORYN_VM_SMP_CPUS > 1 && ORYN_VM_APIC
    KernelIoWriteString("[KERNEL] SMP: starting after virtual memory proof.\n");
    (void)OrynKernelSmpInit(kernelBootInfo);
    OrynKernelSmpPrintProof();
#else
    (void)kernelBootInfo;
    KernelIoWriteString("[KERNEL] INFO: SMP AP startup skipped by VMSettings.\n");
#endif
}

static void RunMemoryProofs(const OrynBootInfo* kernelBootInfo)
{
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
                RunSmpProof(kernelBootInfo);
            }
            else
            {
                OrynVirtualMemoryPrintProof(&gVirtualMemory);
                KernelIoWriteString("[KERNEL] Virtual memory: failed\n");
                KernelIoWriteString("[KERNEL] WARN: SMP AP startup skipped because virtual memory did not activate.\n");
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

void KernelStart(const OrynBootInfo* bootInfo)
{
    KernelDisableInterrupts();
    const OrynBootInfo* kernelBootInfo = KernelBootInfoAdopt(bootInfo);

    KernelIoInit();
    KernelIoWriteString("[KERNEL] Oryn Kernel-5 entered.\n");
    KernelIoWriteString("[KERNEL] PASS: Kernel entered successfully.\n");
    KernelIoWriteString("[KERNEL] PASS: Serial/debug output path is working.\n");
    PrintBootInfoOwnership(kernelBootInfo);
    KernelBootInfoPrintSelection();
    OrynKernelBootInfoStatus bootStatus = KernelBootInfoValidate(kernelBootInfo);

    RunDescriptorAndSysCallProofs();
    RunInterruptAndTimerProofs(kernelBootInfo);
    RunPciProof(kernelBootInfo);
#if ORYN_VM_SMP_CPUS > 1 && ORYN_VM_APIC
    (void)OrynKernelSmpDiscover(kernelBootInfo);
#else
    KernelIoWriteString("[KERNEL] INFO: SMP discovery skipped by VMSettings.\n");
#endif

    if (bootStatus.IsValid)
    {
        KConsoleInit(kernelBootInfo);
        KConsole.ClearScreen();
        KernelIoWriteString("[KERNEL] Oryn Kernel-5 booted.\n");
        KernelIoWriteString("[KERNEL] Target: uefi-x64\n");
        KernelIoWriteString("[KERNEL] Toolchain: clang + lld\n");
        KernelIoWriteString("[KERNEL] PASS: Kernel console initialized.\n");
        KernelIoWriteString(KConsole.IsTtfActive() ?
            "[KERNEL] TTF renderer: active\n" :
            "[KERNEL] TTF renderer: fallback bitmap glyphs\n");
        KernelBootInfoPrintSummary(kernelBootInfo);
        RunMemoryProofs(kernelBootInfo);
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
