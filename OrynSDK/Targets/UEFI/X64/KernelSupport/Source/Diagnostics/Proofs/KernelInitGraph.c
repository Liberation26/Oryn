#include "KernelInitGraph.h"

static const OrynKernelInitStage gKernelInitStages[] =
{
    { OrynKernelInitStageEntry, "Entry", "Enter kernel runtime from loader handoff", 0U, { OrynKernelInitStageEntry } },
    { OrynKernelInitStageBootInfo, "BootInfo", "Adopt and validate supplied BootInfo", 1U, { OrynKernelInitStageEntry } },
    { OrynKernelInitStageModuleManifest, "ModuleManifest", "Load generated module manifests and requested module needs", 1U, { OrynKernelInitStageBootInfo } },
    { OrynKernelInitStageLibC, "LibC", "Run freestanding LibC self proof", 1U, { OrynKernelInitStageModuleManifest } },
    { OrynKernelInitStageDescriptors, "Descriptors", "Install GDT, IDT, and syscall descriptors", 1U, { OrynKernelInitStageLibC } },
    { OrynKernelInitStageInterrupts, "Interrupts", "Start PIC/APIC interrupt ownership", 1U, { OrynKernelInitStageDescriptors } },
    { OrynKernelInitStageTimers, "Timers", "Start RTC/PIT/HPET/APIC timer ownership", 1U, { OrynKernelInitStageInterrupts } },
    { OrynKernelInitStagePci, "PCI", "Discover PCI devices when requested", 1U, { OrynKernelInitStageTimers } },
    { OrynKernelInitStageConsole, "Console", "Start runtime console when requested", 2U, { OrynKernelInitStageBootInfo, OrynKernelInitStageTimers } },
    { OrynKernelInitStageFat32Vfs, "FAT32/VFS", "Mount filesystem modules when requested", 2U, { OrynKernelInitStageBootInfo, OrynKernelInitStageConsole } },
    { OrynKernelInitStageMemory, "Memory", "Start memory map, physical allocator, and virtual memory", 2U, { OrynKernelInitStageBootInfo, OrynKernelInitStageDescriptors } },
    { OrynKernelInitStageRunning, "Running", "Enter runtime running state after required proofs", 3U, { OrynKernelInitStageDescriptors, OrynKernelInitStageInterrupts, OrynKernelInitStageTimers } }
};

unsigned int OrynKernelInitGraphCount(void)
{
    return (unsigned int)(sizeof(gKernelInitStages) / sizeof(gKernelInitStages[0]));
}

const OrynKernelInitStage* OrynKernelInitGraphGet(unsigned int index)
{
    if (index >= OrynKernelInitGraphCount())
    {
        return 0;
    }

    return &gKernelInitStages[index];
}

const char* OrynKernelInitStageName(OrynKernelInitStageId id)
{
    unsigned int index = 0;
    for (index = 0; index < OrynKernelInitGraphCount(); ++index)
    {
        if (gKernelInitStages[index].Id == id)
        {
            return gKernelInitStages[index].Name;
        }
    }

    return "Unknown";
}

static int OrynKernelInitGraphFindOrder(OrynKernelInitStageId id)
{
    unsigned int index = 0;
    for (index = 0; index < OrynKernelInitGraphCount(); ++index)
    {
        if (gKernelInitStages[index].Id == id)
        {
            return (int)index;
        }
    }

    return -1;
}

int OrynKernelInitGraphValidate(void)
{
    unsigned int index = 0;
    for (index = 0; index < OrynKernelInitGraphCount(); ++index)
    {
        const OrynKernelInitStage* stage = &gKernelInitStages[index];
        unsigned int depIndex = 0;
        if (stage->DependencyCount > 4U)
        {
            return 0;
        }

        for (depIndex = 0; depIndex < stage->DependencyCount; ++depIndex)
        {
            int dependencyOrder = OrynKernelInitGraphFindOrder(stage->Dependencies[depIndex]);
            if (dependencyOrder < 0 || dependencyOrder >= (int)index)
            {
                return 0;
            }
        }
    }

    return 1;
}
