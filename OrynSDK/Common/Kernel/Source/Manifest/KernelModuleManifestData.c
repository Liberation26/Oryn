#include "KernelModuleManifest.h"

/* Generated from kernel module manifest files. Do not hand-edit module tables here. */

int OrynKernelModuleDefaultStop(OrynKernelModuleId id)
{
    (void)id;
    return 1;
}

int OrynKernelModuleDefaultPanic(OrynKernelModuleId id)
{
    (void)id;
    return 1;
}

int OrynKernelModuleDefaultShutdown(OrynKernelModuleId id)
{
    (void)id;
    return 1;
}

static OrynKernelModuleManifestItem gKernelModuleManifest[OrynKernelModuleCount];
static OrynKernelCompiledModuleRecord gCompiledKernelModules[OrynKernelModuleCount];

static void SetModule(
    OrynKernelModuleId id,
    const char* name,
    const char* items,
    const char* selects,
    const OrynKernelModuleId* requires,
    unsigned int requireCount,
    int compiledIn,
    int required,
    int fatalOnMissingPrerequisite,
    const char* stopCallbackName,
    const char* panicCallbackName,
    const char* shutdownCallbackName,
    OrynKernelModuleLifecycleCallback stopCallback,
    OrynKernelModuleLifecycleCallback panicCallback,
    OrynKernelModuleLifecycleCallback shutdownCallback)
{
    gKernelModuleManifest[id].Id = id;
    gKernelModuleManifest[id].Name = name;
    gKernelModuleManifest[id].Items = items;
    gKernelModuleManifest[id].Selects = selects;
    gKernelModuleManifest[id].RequireCount = requireCount;
    gKernelModuleManifest[id].CompiledIn = compiledIn;
    gKernelModuleManifest[id].Required = required;
    gKernelModuleManifest[id].FatalOnMissingPrerequisite = fatalOnMissingPrerequisite;
    gKernelModuleManifest[id].StopCallbackName = stopCallbackName;
    gKernelModuleManifest[id].PanicCallbackName = panicCallbackName;
    gKernelModuleManifest[id].ShutdownCallbackName = shutdownCallbackName;
    gKernelModuleManifest[id].StopCallback = stopCallback;
    gKernelModuleManifest[id].PanicCallback = panicCallback;
    gKernelModuleManifest[id].ShutdownCallback = shutdownCallback;
    gKernelModuleManifest[id].State = compiledIn ? OrynKernelModuleStateRegistered : OrynKernelModuleStateAbsent;
    gCompiledKernelModules[id].Id = id;
    gCompiledKernelModules[id].Name = name;
    gCompiledKernelModules[id].CompiledIn = compiledIn;
    for (unsigned int index = 0U; index < requireCount && index < 6U; ++index)
    {
        gKernelModuleManifest[id].Requires[index] = requires[index];
    }
}

void OrynKernelModuleManifestInit(void)
{
    static const OrynKernelModuleId requires_OrynKernelModuleBootInfo[] = { OrynKernelModuleCount };
    static const OrynKernelModuleId requires_OrynKernelModuleScreenReport[] = { OrynKernelModuleCount };
    static const OrynKernelModuleId requires_OrynKernelModuleLibC[] = { OrynKernelModuleCount };
    static const OrynKernelModuleId requires_OrynKernelModuleLifecycle[] = { OrynKernelModuleCount };
    static const OrynKernelModuleId requires_OrynKernelModulePanic[] = { OrynKernelModuleBootInfo };
    static const OrynKernelModuleId requires_OrynKernelModuleGdt[] = { OrynKernelModuleCount };
    static const OrynKernelModuleId requires_OrynKernelModuleIdt[] = { OrynKernelModuleGdt };
    static const OrynKernelModuleId requires_OrynKernelModuleInterrupts[] = { OrynKernelModuleGdt, OrynKernelModuleIdt };
    static const OrynKernelModuleId requires_OrynKernelModuleSysCalls[] = { OrynKernelModuleGdt, OrynKernelModuleIdt };
    static const OrynKernelModuleId requires_OrynKernelModuleCpu[] = { OrynKernelModuleCount };
    static const OrynKernelModuleId requires_OrynKernelModulePic[] = { OrynKernelModuleCpu, OrynKernelModuleInterrupts };
    static const OrynKernelModuleId requires_OrynKernelModuleApic[] = { OrynKernelModuleCpu, OrynKernelModuleInterrupts, OrynKernelModulePic };
    static const OrynKernelModuleId requires_OrynKernelModuleSmp[] = { OrynKernelModuleApic };
    static const OrynKernelModuleId requires_OrynKernelModuleHpet[] = { OrynKernelModuleBootInfo, OrynKernelModuleInterrupts };
    static const OrynKernelModuleId requires_OrynKernelModulePci[] = { OrynKernelModuleBootInfo };
    static const OrynKernelModuleId requires_OrynKernelModuleConsole[] = { OrynKernelModuleBootInfo };
    static const OrynKernelModuleId requires_OrynKernelModuleKeyboard[] = { OrynKernelModuleConsole, OrynKernelModuleInterrupts, OrynKernelModulePic };
    static const OrynKernelModuleId requires_OrynKernelModuleFat32[] = { OrynKernelModuleConsole };
    static const OrynKernelModuleId requires_OrynKernelModuleVfs[] = { OrynKernelModuleFat32 };
    static const OrynKernelModuleId requires_OrynKernelModulePhysicalMemory[] = { OrynKernelModuleBootInfo };
    static const OrynKernelModuleId requires_OrynKernelModuleHeap[] = { OrynKernelModulePhysicalMemory };
    static const OrynKernelModuleId requires_OrynKernelModuleVirtualMemory[] = { OrynKernelModuleHeap };
    static const OrynKernelModuleId requires_OrynKernelModuleProcess[] = { OrynKernelModuleVirtualMemory };
    static const OrynKernelModuleId requires_OrynKernelModuleScheduler[] = { OrynKernelModuleProcess, OrynKernelModuleHeap };

    SetModule(OrynKernelModuleBootInfo, "BootInfo", "ABI, checksum, command line, memory map, framebuffer", "Always", requires_OrynKernelModuleBootInfo, 0U, 1, 1, 1, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModuleScreenReport, "KernelScreenReport", "OK, WARN, FAIL, category emission, colour", "Always", requires_OrynKernelModuleScreenReport, 0U, 1, 1, 1, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModuleLibC, "OrynLibC", "stddef, stdint, string, ctype, stdlib, errno, assert", "Always", requires_OrynKernelModuleLibC, 0U, 1, 1, 1, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModuleLifecycle, "Lifecycle", "state transitions, history, invalid transition count", "Always", requires_OrynKernelModuleLifecycle, 0U, 1, 1, 1, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModulePanic, "Panic", "panic begin, report, active state, halt, unhandled exception crash policy", "Always", requires_OrynKernelModulePanic, 1U, 1, 1, 1, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModuleGdt, "GDT", "kernel code/data selectors, TSS descriptor, IST stack", "Always", requires_OrynKernelModuleGdt, 0U, 1, 1, 1, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModuleIdt, "IDT", "256 gates, exception stubs, interrupt stubs", "Always", requires_OrynKernelModuleIdt, 1U, 1, 1, 1, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModuleInterrupts, "Interrupts", "dispatcher, handler table, vector handlers, device handlers, PIC/APIC timer vectors, IRQ mask APIs, per-CPU accounting", "Always", requires_OrynKernelModuleInterrupts, 2U, 1, 1, 1, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModuleSysCalls, "SysCalls", "Get, Set, Event, ABI versioning, argument validation, user pointer validation, credential placeholder, errno mapping, tracing, fuzz tests, Linux/MS translators", "Always", requires_OrynKernelModuleSysCalls, 2U, 1, 1, 1, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModuleCpu, "CPU", "CPUID vendor, APIC, x2APIC, TSC deadline", "Always", requires_OrynKernelModuleCpu, 0U, 1, 1, 1, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModulePic, "PIC", "remap, masks, IRQ0 proof, EOI", "VmPic", requires_OrynKernelModulePic, 2U, 1, 0, 1, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModuleApic, "APIC", "local APIC, x2APIC, IOAPIC discovery, IRQ routing, timer, EOI, IPI", "VmApic", requires_OrynKernelModuleApic, 3U, 1, 0, 1, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModuleSmp, "SMP", "MADT topology, AP trampoline, INIT/SIPI", "VmSmp,BootInfoRsdp", requires_OrynKernelModuleSmp, 1U, 1, 0, 0, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModuleHpet, "HPET", "ACPI HPET table, MMIO base, main counter", "VmHpet,BootInfoRsdp", requires_OrynKernelModuleHpet, 2U, 1, 0, 0, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModulePci, "PCI", "bus scan, config space, class decode, MSI, MSI-X, device interrupt records", "BootInfoRsdp", requires_OrynKernelModulePci, 1U, 1, 0, 0, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModuleConsole, "Console", "framebuffer, scrollback, double buffer, glyphs", "BootInfoFramebuffer", requires_OrynKernelModuleConsole, 1U, 1, 0, 0, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModuleKeyboard, "Keyboard", "IRQ1, scan decoder, scroll keys, release state", "BootInfoFramebuffer,VmPic", requires_OrynKernelModuleKeyboard, 3U, 1, 0, 0, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModuleFat32, "FAT32", "BPB, FSInfo, FAT, clusters, directories, files", "BootInfoFramebuffer", requires_OrynKernelModuleFat32, 1U, 1, 0, 0, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModuleVfs, "VFS", "root mount, stat, read, write, delete, list, ELF64 user command loading", "BootInfoFramebuffer", requires_OrynKernelModuleVfs, 1U, 1, 0, 0, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModulePhysicalMemory, "PhysicalMemory", "usable pages, free list, allocation proof, DMA-safe constraints, contiguous constrained allocations, memory pressure, out-of-memory policy", "BootInfoMemoryMap", requires_OrynKernelModulePhysicalMemory, 1U, 1, 0, 1, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModuleHeap, "Heap", "kmalloc, kfree, krealloc, kcalloc, stats, leak counters, slab caches, guard pages", "BootInfoMemoryMap", requires_OrynKernelModuleHeap, 1U, 1, 0, 1, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModuleVirtualMemory, "VirtualMemory", "PML4, CR3 switch, identity and higher-half maps, copy-on-write clone and fault resolution, mmap-style regions, W^X page policy", "BootInfoMemoryMap,BootInfoKernelRange", requires_OrynKernelModuleVirtualMemory, 1U, 1, 0, 1, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModuleProcess, "Process", "process IDs, thread IDs, parent-child relationships, exit status, wait semantics, Oryn Event signal delivery, kernel thread structure, user process structure, user thread structure, per-process address spaces, ownership, lifecycle state, copy-on-write child foundation", "BootInfoMemoryMap,BootInfoKernelRange", requires_OrynKernelModuleProcess, 1U, 1, 0, 1, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
    SetModule(OrynKernelModuleScheduler, "Scheduler", "priorities, thread states, wait/wake primitives, per-CPU idle thread registration, kernel thread records, scheduler-ready stacks, guarded stack allocation, timer wheel, sleep queues, work queues, wait queues, per-CPU run queues, pre-emptive round-robin scheduler, CPU affinity, scheduler diagnostics, run-queue dumps", "BootInfoMemoryMap,BootInfoKernelRange", requires_OrynKernelModuleScheduler, 2U, 1, 0, 1, "OrynKernelModuleDefaultStop", "OrynKernelModuleDefaultPanic", "OrynKernelModuleDefaultShutdown", OrynKernelModuleDefaultStop, OrynKernelModuleDefaultPanic, OrynKernelModuleDefaultShutdown);
}

OrynKernelModuleManifestItem* OrynKernelModuleManifestMutable(OrynKernelModuleId id)
{
    if ((unsigned int)id >= (unsigned int)OrynKernelModuleCount)
    {
        return 0;
    }

    return &gKernelModuleManifest[id];
}

const OrynKernelModuleManifestItem* OrynKernelModuleManifestGet(OrynKernelModuleId id)
{
    return OrynKernelModuleManifestMutable(id);
}

unsigned int OrynKernelCompiledModuleCount(void)
{
    return (unsigned int)OrynKernelModuleCount;
}

const OrynKernelCompiledModuleRecord* OrynKernelCompiledModuleGet(unsigned int index)
{
    if (index >= (unsigned int)OrynKernelModuleCount)
    {
        return 0;
    }
    return &gCompiledKernelModules[index];
}
