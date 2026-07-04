#include "KernelHeapInternal.h"

static unsigned long long AlignGuardDown(unsigned long long value)
{
    return value & ~(ORYN_KERNEL_HEAP_GUARD_PAGE_SIZE - 1ULL);
}

static unsigned int TryUnmapGuard(unsigned long long guardAddress)
{
    if (guardAddress == 0ULL)
    {
        return 0U;
    }
    if (gOrynHeapVirtualMemory == 0)
    {
        return 0U;
    }
    return OrynVirtualMemoryUnmapGuardPage(guardAddress) ? 1U : 0U;
}

int OrynHeapRecordGuardPage(
    unsigned int kind,
    unsigned long long guardAddress,
    unsigned long long protectedBase,
    unsigned long long protectedBytes,
    unsigned int unmapped)
{
    if (guardAddress == 0ULL || protectedBase == 0ULL || protectedBytes == 0ULL)
    {
        gOrynHeapStats.GuardInstallFailures += 1ULL;
        return 0;
    }

    for (unsigned int index = 0U; index < ORYN_KERNEL_HEAP_GUARD_RECORD_COUNT; ++index)
    {
        OrynKernelHeapGuardRecord* record = &gOrynHeapGuardRecords[index];
        if (record->Used == 0U)
        {
            record->Used = 1U;
            record->Kind = kind;
            record->GuardAddress = guardAddress;
            record->ProtectedBase = protectedBase;
            record->ProtectedBytes = protectedBytes;
            record->Unmapped = unmapped;
            gOrynHeapStats.GuardPages += 1ULL;
            if (unmapped != 0U)
            {
                gOrynHeapStats.GuardPagesUnmapped += 1ULL;
            }
            if (kind == ORYN_KERNEL_HEAP_GUARD_KIND_STACK)
            {
                gOrynHeapStats.StackGuardPages += 1ULL;
            }
            else if (kind == ORYN_KERNEL_HEAP_GUARD_KIND_CRITICAL)
            {
                gOrynHeapStats.CriticalHeapGuardPages += 1ULL;
            }
            else
            {
                gOrynHeapStats.GuardInstallFailures += 1ULL;
                return 0;
            }
            return 1;
        }
    }

    gOrynHeapStats.GuardInstallFailures += 1ULL;
    return 0;
}

void OrynKernelHeapInstallStackGuard(unsigned long long stackBase, unsigned long long stackBytes)
{
    unsigned long long guardAddress;
    unsigned int unmapped;
    if (stackBase == 0ULL || stackBytes < ORYN_KERNEL_HEAP_GUARD_PAGE_SIZE)
    {
        gOrynHeapStats.GuardInstallFailures += 1ULL;
        return;
    }
    guardAddress = AlignGuardDown(stackBase);
    unmapped = TryUnmapGuard(guardAddress);
    (void)OrynHeapRecordGuardPage(
        ORYN_KERNEL_HEAP_GUARD_KIND_STACK,
        guardAddress,
        stackBase,
        stackBytes,
        unmapped);
}

void* OrynKernelHeapAllocCritical(unsigned long long size)
{
    void* allocation;
    unsigned long long guardPage;
    unsigned int unmapped;

    allocation = OrynHeapAllocateRaw(size, ORYN_KERNEL_HEAP_FLAG_CRITICAL);
    if (allocation == 0)
    {
        return 0;
    }

    guardPage = OrynHeapAllocatePage();
    if (guardPage == ORYN_PHYSICAL_ALLOC_FAIL)
    {
        gOrynHeapStats.GuardInstallFailures += 1ULL;
        return allocation;
    }

    (void)OrynPhysicalMemorySetPageOwner(
        gOrynHeapPhysicalMemory,
        guardPage,
        OrynPhysicalPageOwnerKernelHeap,
        ORYN_KERNEL_HEAP_FLAG_CRITICAL | ORYN_VIRTUAL_FLAG_GUARD);

    unmapped = TryUnmapGuard(guardPage);
    (void)OrynHeapRecordGuardPage(
        ORYN_KERNEL_HEAP_GUARD_KIND_CRITICAL,
        guardPage,
        (unsigned long long)allocation,
        size,
        unmapped);
    return allocation;
}

int OrynKernelHeapValidateGuards(void)
{
    unsigned long long total = 0ULL;
    unsigned long long stacks = 0ULL;
    unsigned long long critical = 0ULL;
    unsigned long long unmapped = 0ULL;
    int ok = 1;

    gOrynHeapStats.GuardValidationRuns += 1ULL;
    for (unsigned int index = 0U; index < ORYN_KERNEL_HEAP_GUARD_RECORD_COUNT; ++index)
    {
        OrynKernelHeapGuardRecord* record = &gOrynHeapGuardRecords[index];
        if (record->Used == 0U)
        {
            continue;
        }
        if (record->GuardAddress == 0ULL ||
            record->ProtectedBase == 0ULL ||
            record->ProtectedBytes == 0ULL ||
            (record->GuardAddress & (ORYN_KERNEL_HEAP_GUARD_PAGE_SIZE - 1ULL)) != 0ULL)
        {
            ok = 0;
        }
        total += 1ULL;
        if (record->Unmapped != 0U)
        {
            unmapped += 1ULL;
        }
        if (record->Kind == ORYN_KERNEL_HEAP_GUARD_KIND_STACK)
        {
            stacks += 1ULL;
        }
        else if (record->Kind == ORYN_KERNEL_HEAP_GUARD_KIND_CRITICAL)
        {
            critical += 1ULL;
        }
        else
        {
            ok = 0;
        }
    }

    if (total != gOrynHeapStats.GuardPages ||
        stacks != gOrynHeapStats.StackGuardPages ||
        critical != gOrynHeapStats.CriticalHeapGuardPages ||
        unmapped != gOrynHeapStats.GuardPagesUnmapped)
    {
        ok = 0;
    }
    if (!ok)
    {
        gOrynHeapStats.GuardValidationFailures += 1ULL;
    }
    return ok;
}
