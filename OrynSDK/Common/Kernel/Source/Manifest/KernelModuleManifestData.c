#include "KernelModuleManifest.h"

/* Generated from kernel module manifest files. Do not hand-edit module tables here. */

static OrynKernelModuleManifestItem gKernelModuleManifest[OrynKernelModuleCount];

static void SetModule(
    OrynKernelModuleId id,
    const char* name,
    const char* items,
    const OrynKernelModuleId* requires,
    unsigned int requireCount)
{
    gKernelModuleManifest[id].Id = id;
    gKernelModuleManifest[id].Name = name;
    gKernelModuleManifest[id].Items = items;
    gKernelModuleManifest[id].RequireCount = requireCount;
    gKernelModuleManifest[id].State = OrynKernelModuleStateRegistered;
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
    static const OrynKernelModuleId requires_OrynKernelModuleVirtualMemory[] = { OrynKernelModulePhysicalMemory };

    SetModule(OrynKernelModuleBootInfo, "BootInfo", "ABI, checksum, command line, memory map, framebuffer", requires_OrynKernelModuleBootInfo, 0U);
    SetModule(OrynKernelModuleScreenReport, "KernelScreenReport", "OK, WARN, FAIL, category emission, colour", requires_OrynKernelModuleScreenReport, 0U);
    SetModule(OrynKernelModuleLibC, "OrynLibC", "stddef, stdint, string, ctype, stdlib, errno, assert", requires_OrynKernelModuleLibC, 0U);
    SetModule(OrynKernelModuleLifecycle, "Lifecycle", "state transitions, history, invalid transition count", requires_OrynKernelModuleLifecycle, 0U);
    SetModule(OrynKernelModulePanic, "Panic", "panic begin, report, active state, halt", requires_OrynKernelModulePanic, 1U);
    SetModule(OrynKernelModuleGdt, "GDT", "kernel code/data selectors, TSS descriptor, IST stack", requires_OrynKernelModuleGdt, 0U);
    SetModule(OrynKernelModuleIdt, "IDT", "256 gates, exception stubs, interrupt stubs", requires_OrynKernelModuleIdt, 1U);
    SetModule(OrynKernelModuleInterrupts, "Interrupts", "dispatcher, handler table, PIC/APIC timer vectors", requires_OrynKernelModuleInterrupts, 2U);
    SetModule(OrynKernelModuleSysCalls, "SysCalls", "Get, Set, Event, Linux/MS translators", requires_OrynKernelModuleSysCalls, 2U);
    SetModule(OrynKernelModuleCpu, "CPU", "CPUID vendor, APIC, x2APIC, TSC deadline", requires_OrynKernelModuleCpu, 0U);
    SetModule(OrynKernelModulePic, "PIC", "remap, masks, IRQ0 proof, EOI", requires_OrynKernelModulePic, 2U);
    SetModule(OrynKernelModuleApic, "APIC", "local APIC, x2APIC, timer, EOI, IPI", requires_OrynKernelModuleApic, 3U);
    SetModule(OrynKernelModuleSmp, "SMP", "MADT topology, AP trampoline, INIT/SIPI", requires_OrynKernelModuleSmp, 1U);
    SetModule(OrynKernelModuleHpet, "HPET", "ACPI HPET table, MMIO base, main counter", requires_OrynKernelModuleHpet, 2U);
    SetModule(OrynKernelModulePci, "PCI", "MCFG, ECAM, config mechanism, device report", requires_OrynKernelModulePci, 1U);
    SetModule(OrynKernelModuleConsole, "Console", "framebuffer, scrollback, double buffer, glyphs", requires_OrynKernelModuleConsole, 1U);
    SetModule(OrynKernelModuleKeyboard, "Keyboard", "IRQ1, scan decoder, scroll keys, release state", requires_OrynKernelModuleKeyboard, 3U);
    SetModule(OrynKernelModuleFat32, "FAT32", "BPB, FSInfo, FAT, clusters, directories, files", requires_OrynKernelModuleFat32, 1U);
    SetModule(OrynKernelModuleVfs, "VFS", "root mount, stat, read, write, delete, list", requires_OrynKernelModuleVfs, 1U);
    SetModule(OrynKernelModulePhysicalMemory, "PhysicalMemory", "usable pages, free list, allocation proof", requires_OrynKernelModulePhysicalMemory, 1U);
    SetModule(OrynKernelModuleVirtualMemory, "VirtualMemory", "PML4, CR3 switch, identity and higher-half maps", requires_OrynKernelModuleVirtualMemory, 1U);
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
