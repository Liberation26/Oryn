#include "KernelModuleManifest.h"

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
    static const OrynKernelModuleId none[] = { OrynKernelModuleCount };
    static const OrynKernelModuleId boot[] = { OrynKernelModuleBootInfo };
    static const OrynKernelModuleId desc[] = { OrynKernelModuleGdt };
    static const OrynKernelModuleId intr[] = { OrynKernelModuleGdt, OrynKernelModuleIdt };
    static const OrynKernelModuleId pic[] = { OrynKernelModuleCpu, OrynKernelModuleInterrupts };
    static const OrynKernelModuleId apic[] = { OrynKernelModuleCpu, OrynKernelModuleInterrupts, OrynKernelModulePic };
    static const OrynKernelModuleId smp[] = { OrynKernelModuleApic };
    static const OrynKernelModuleId hpet[] = { OrynKernelModuleBootInfo, OrynKernelModuleInterrupts };
    static const OrynKernelModuleId pci[] = { OrynKernelModuleBootInfo };
    static const OrynKernelModuleId console[] = { OrynKernelModuleBootInfo };
    static const OrynKernelModuleId keyboard[] = { OrynKernelModuleConsole, OrynKernelModuleInterrupts, OrynKernelModulePic };
    static const OrynKernelModuleId fat32[] = { OrynKernelModuleConsole };
    static const OrynKernelModuleId vfs[] = { OrynKernelModuleFat32 };
    static const OrynKernelModuleId pmem[] = { OrynKernelModuleBootInfo };
    static const OrynKernelModuleId vmem[] = { OrynKernelModulePhysicalMemory };

    SetModule(OrynKernelModuleBootInfo, "BootInfo", "ABI, checksum, command line, memory map, framebuffer", none, 0U);
    SetModule(OrynKernelModuleScreenReport, "KernelScreenReport", "OK, WARN, FAIL, category emission, colour", none, 0U);
    SetModule(OrynKernelModuleLifecycle, "Lifecycle", "state transitions, history, invalid transition count", none, 0U);
    SetModule(OrynKernelModulePanic, "Panic", "panic begin, report, active state, halt", boot, 1U);
    SetModule(OrynKernelModuleGdt, "GDT", "kernel code/data selectors, TSS descriptor, IST stack", none, 0U);
    SetModule(OrynKernelModuleIdt, "IDT", "256 gates, exception stubs, interrupt stubs", desc, 1U);
    SetModule(OrynKernelModuleInterrupts, "Interrupts", "dispatcher, handler table, PIC/APIC timer vectors", intr, 2U);
    SetModule(OrynKernelModuleSysCalls, "SysCalls", "Get, Set, Event, Linux/MS translators", intr, 2U);
    SetModule(OrynKernelModuleCpu, "CPU", "CPUID vendor, APIC, x2APIC, TSC deadline", none, 0U);
    SetModule(OrynKernelModulePic, "PIC", "remap, masks, IRQ0 proof, EOI", pic, 2U);
    SetModule(OrynKernelModuleApic, "APIC", "local APIC, x2APIC, timer, EOI, IPI", apic, 3U);
    SetModule(OrynKernelModuleSmp, "SMP", "MADT topology, AP trampoline, INIT/SIPI", smp, 1U);
    SetModule(OrynKernelModuleHpet, "HPET", "ACPI HPET table, MMIO base, main counter", hpet, 2U);
    SetModule(OrynKernelModulePci, "PCI", "MCFG, ECAM, config mechanism, device report", pci, 1U);
    SetModule(OrynKernelModuleConsole, "Console", "framebuffer, scrollback, double buffer, glyphs", console, 1U);
    SetModule(OrynKernelModuleKeyboard, "Keyboard", "IRQ1, scan decoder, scroll keys, release state", keyboard, 3U);
    SetModule(OrynKernelModuleFat32, "FAT32", "BPB, FSInfo, FAT, clusters, directories, files", fat32, 1U);
    SetModule(OrynKernelModuleVfs, "VFS", "root mount, stat, read, write, delete, list", vfs, 1U);
    SetModule(OrynKernelModulePhysicalMemory, "PhysicalMemory", "usable pages, free list, allocation proof", pmem, 1U);
    SetModule(OrynKernelModuleVirtualMemory, "VirtualMemory", "PML4, CR3 switch, identity and higher-half maps", vmem, 1U);
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
