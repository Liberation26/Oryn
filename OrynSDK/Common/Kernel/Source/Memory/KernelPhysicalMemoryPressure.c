#include "KernelPhysicalMemory.h"

static unsigned int PercentOf(unsigned int value, unsigned int percent)
{
    unsigned long long product = (unsigned long long)value * (unsigned long long)percent;
    unsigned int pages = (unsigned int)(product / 100ULL);
    return pages == 0U && value != 0U ? 1U : pages;
}

static unsigned int ChooseLevel(const OrynKernelPhysicalMemory* allocator)
{
    if (allocator == 0 || allocator->Initialized == 0U)
    {
        return OrynPhysicalMemoryPressureOutOfMemory;
    }
    if (allocator->FreePageCount == 0U)
    {
        return OrynPhysicalMemoryPressureOutOfMemory;
    }
    if (allocator->FreePageCount <= allocator->Pressure.CriticalWatermarkPages)
    {
        return OrynPhysicalMemoryPressureCritical;
    }
    if (allocator->FreePageCount <= allocator->Pressure.LowWatermarkPages)
    {
        return OrynPhysicalMemoryPressureLow;
    }
    return OrynPhysicalMemoryPressureNormal;
}

void OrynPhysicalMemoryPressureConfigure(
    OrynKernelPhysicalMemory* allocator,
    unsigned int lowWatermarkPages,
    unsigned int criticalWatermarkPages)
{
    if (allocator == 0)
    {
        return;
    }
    if (lowWatermarkPages == 0U)
    {
        lowWatermarkPages = PercentOf(allocator->TrackedUsablePages, ORYN_PHYSICAL_PRESSURE_LOW_PERCENT);
    }
    if (criticalWatermarkPages == 0U)
    {
        criticalWatermarkPages = PercentOf(
            allocator->TrackedUsablePages,
            ORYN_PHYSICAL_PRESSURE_CRITICAL_PERCENT);
    }
    if (criticalWatermarkPages > lowWatermarkPages)
    {
        criticalWatermarkPages = lowWatermarkPages;
    }
    allocator->Pressure.Initialized = 1U;
    allocator->Pressure.LowWatermarkPages = lowWatermarkPages;
    allocator->Pressure.CriticalWatermarkPages = criticalWatermarkPages;
    OrynPhysicalMemoryPressureRefresh(allocator);
}

void OrynPhysicalMemoryPressureRefresh(OrynKernelPhysicalMemory* allocator)
{
    unsigned int oldLevel;
    unsigned int newLevel;
    if (allocator == 0)
    {
        return;
    }
    if (allocator->Pressure.Initialized == 0U)
    {
        OrynPhysicalMemoryPressureConfigure(allocator, 0U, 0U);
        return;
    }
    oldLevel = allocator->Pressure.Level;
    newLevel = ChooseLevel(allocator);
    allocator->Pressure.Level = newLevel;
    allocator->Pressure.FreePages = allocator->FreePageCount;
    allocator->Pressure.UsedPages = allocator->UsedPageCount;
    allocator->Pressure.TotalTrackedPages = allocator->TrackedUsablePages;
    if (oldLevel < OrynPhysicalMemoryPressureLow && newLevel >= OrynPhysicalMemoryPressureLow)
    {
        allocator->Pressure.LowTransitions += 1ULL;
    }
    if (oldLevel < OrynPhysicalMemoryPressureCritical && newLevel >= OrynPhysicalMemoryPressureCritical)
    {
        allocator->Pressure.CriticalTransitions += 1ULL;
    }
}

const OrynPhysicalMemoryPressureState* OrynPhysicalMemoryGetPressureState(
    const OrynKernelPhysicalMemory* allocator)
{
    if (allocator == 0)
    {
        return 0;
    }
    return &allocator->Pressure;
}

unsigned int OrynPhysicalMemoryOutOfMemoryAction(
    const OrynKernelPhysicalMemory* allocator,
    unsigned int kernelRequest)
{
    unsigned int level = allocator == 0 ? OrynPhysicalMemoryPressureOutOfMemory : allocator->Pressure.Level;
    if (level == OrynPhysicalMemoryPressureOutOfMemory)
    {
        return kernelRequest != 0U ?
            OrynPhysicalOutOfMemoryActionKernelPanic : OrynPhysicalOutOfMemoryActionKillProcess;
    }
    if (level == OrynPhysicalMemoryPressureCritical)
    {
        return OrynPhysicalOutOfMemoryActionReclaim;
    }
    if (level == OrynPhysicalMemoryPressureLow)
    {
        return OrynPhysicalOutOfMemoryActionReclaim;
    }
    return OrynPhysicalOutOfMemoryActionContinue;
}

int OrynPhysicalMemoryRunPressureSelfTest(OrynKernelPhysicalMemory* allocator)
{
    unsigned int savedLow;
    unsigned int savedCritical;
    unsigned int savedLevel;
    if (allocator == 0 || allocator->Initialized == 0U)
    {
        return 0;
    }
    savedLow = allocator->Pressure.LowWatermarkPages;
    savedCritical = allocator->Pressure.CriticalWatermarkPages;
    savedLevel = allocator->Pressure.Level;
    OrynPhysicalMemoryPressureConfigure(
        allocator,
        allocator->FreePageCount + 1U,
        allocator->FreePageCount + 1U);
    if (allocator->Pressure.Level < OrynPhysicalMemoryPressureCritical)
    {
        return 0;
    }
    OrynPhysicalMemoryPressureConfigure(allocator, savedLow, savedCritical);
    allocator->Pressure.Level = savedLevel;
    OrynPhysicalMemoryPressureRefresh(allocator);
    return OrynPhysicalMemoryOutOfMemoryAction(allocator, 0U) !=
        OrynPhysicalOutOfMemoryActionKernelPanic;
}
