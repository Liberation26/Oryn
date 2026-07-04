#include "KernelVirtualMemory.h"
#include "KernelPageFaultPolicy.h"

static void VmSelfTestClearBytes(void* target, unsigned long long count)
{
    unsigned char* bytes = (unsigned char*)target;
    for (unsigned long long index = 0ULL; index < count; ++index)
    {
        bytes[index] = 0U;
    }
}

int OrynVirtualMemoryRunAddressSpaceSelfTest(
    OrynKernelVirtualMemory* virtualMemory,
    OrynKernelPhysicalMemory* physicalMemory)
{
    OrynKernelAddressSpace processSpace;
    OrynKernelAddressSpace childSpace;
    unsigned long long physical;
    unsigned long long userAddress = ORYN_VIRTUAL_USER_BASE + 0x200000ULL;
    unsigned long long demandAddress = ORYN_VIRTUAL_USER_BASE + 0x300000ULL;
    unsigned long long fileAddress = ORYN_VIRTUAL_USER_BASE + 0x400000ULL;
    unsigned long long deviceAddress = ORYN_VIRTUAL_USER_BASE + 0x500000ULL;
    unsigned char userSeed[8];
    unsigned char kernelCopy[8];
    OrynIdtInterruptFrame demandFrame;
    OrynIdtInterruptFrame cowFrame;
    int ok;
    if (virtualMemory == 0 || physicalMemory == 0)
    {
        return 0;
    }
    for (unsigned int index = 0U; index < sizeof(userSeed); ++index)
    {
        userSeed[index] = (unsigned char)(0x41U + index);
        kernelCopy[index] = 0U;
    }
    VmSelfTestClearBytes(&demandFrame, sizeof(demandFrame));
    VmSelfTestClearBytes(&cowFrame, sizeof(cowFrame));
    VmSelfTestClearBytes(&childSpace, sizeof(childSpace));
    demandFrame.ErrorCode = ORYN_PAGE_FAULT_USER | ORYN_PAGE_FAULT_WRITE;
    cowFrame.ErrorCode = ORYN_PAGE_FAULT_PRESENT | ORYN_PAGE_FAULT_USER | ORYN_PAGE_FAULT_WRITE;

    ok = OrynVirtualMemoryInitKernelAddressSpace(virtualMemory);
    ok = ok && OrynVirtualMemoryCreateProcessAddressSpace(physicalMemory, &processSpace);
    ok = ok && OrynVirtualMemoryValidateProcessAddressSpace(
        &processSpace, &virtualMemory->KernelAddressSpace);
    physical = OrynPhysicalMemoryAllocatePageBelow(physicalMemory, ORYN_PHYSICAL_EARLY_DIRECT_MAP_LIMIT);
    ok = ok && physical != ORYN_PHYSICAL_ALLOC_FAIL;
    if (ok)
    {
        ok = OrynVirtualMemoryMap(&processSpace, physicalMemory, userAddress, physical,
            ORYN_VIRTUAL_PAGE_SIZE, ORYN_VIRTUAL_FLAG_READ | ORYN_VIRTUAL_FLAG_WRITE | ORYN_VIRTUAL_FLAG_USER);
    }
    if (ok)
    {
        (void)OrynPhysicalMemorySetPageOwner(
            physicalMemory,
            physical,
            OrynPhysicalPageOwnerUserPage,
            processSpace.AddressSpaceId);
        unsigned char* physicalBytes = (unsigned char*)physical;
        for (unsigned int index = 0U; index < sizeof(userSeed); ++index)
        {
            physicalBytes[index] = userSeed[index];
        }
        ok = OrynCopyFromUser(&processSpace, kernelCopy, (const void*)userAddress, sizeof(kernelCopy));
    }
    if (ok)
    {
        for (unsigned int index = 0U; index < sizeof(userSeed); ++index)
        {
            if (kernelCopy[index] != userSeed[index])
            {
                ok = 0;
            }
        }
    }
    if (ok)
    {
        userSeed[0] = 0x5AU;
        ok = OrynCopyToUser(&processSpace, (void*)userAddress, userSeed, sizeof(userSeed));
    }
    if (ok)
    {
        ok = OrynVirtualMemoryCreateCopyOnWriteClone(physicalMemory, &processSpace, &childSpace);
    }
    if (ok)
    {
        ok = OrynVirtualMemoryValidateProcessAddressSpace(
            &childSpace, &virtualMemory->KernelAddressSpace);
    }
    if (ok)
    {
        OrynKernelPageFaultPolicySetProcessContext(&childSpace);
        OrynKernelPageFaultPolicySetDemandAllocator(&childSpace, physicalMemory);
        ok = OrynKernelPageFaultPolicyHandle(&cowFrame, userAddress) ==
            OrynKernelPageFaultActionRecover;
        OrynKernelPageFaultPolicySetDemandAllocator(0, 0);
        OrynKernelPageFaultPolicySetProcessContext(0);
    }
    if (ok)
    {
        ok = childSpace.CopyOnWriteResolvedPages != 0ULL;
    }
    if (ok)
    {
        ok = OrynVirtualMemoryProtect(&processSpace, userAddress, ORYN_VIRTUAL_PAGE_SIZE,
            ORYN_VIRTUAL_FLAG_READ | ORYN_VIRTUAL_FLAG_USER);
    }
    if (ok)
    {
        ok = OrynVirtualMemoryReserveAnonymousRegion(&processSpace, demandAddress,
            ORYN_VIRTUAL_PAGE_SIZE * 2ULL,
            ORYN_VIRTUAL_FLAG_READ | ORYN_VIRTUAL_FLAG_WRITE | ORYN_VIRTUAL_FLAG_USER);
    }
    if (ok)
    {
        ok = OrynVirtualMemoryReserveMmapRegion(&processSpace, fileAddress,
            ORYN_VIRTUAL_PAGE_SIZE, ORYN_VIRTUAL_FLAG_READ | ORYN_VIRTUAL_FLAG_USER,
            OrynVirtualMmapRegionFile, 10ULL, 4096ULL, 0ULL);
    }
    if (ok)
    {
        ok = OrynVirtualMemoryReserveMmapRegion(&processSpace, deviceAddress,
            ORYN_VIRTUAL_PAGE_SIZE, ORYN_VIRTUAL_FLAG_READ | ORYN_VIRTUAL_FLAG_WRITE | ORYN_VIRTUAL_FLAG_USER,
            OrynVirtualMmapRegionDevice, 11ULL, 0ULL, 0xFEC00000ULL);
    }
    if (ok)
    {
        ok = !OrynVirtualMemoryReserveMmapRegion(&processSpace, deviceAddress + ORYN_VIRTUAL_PAGE_SIZE,
            ORYN_VIRTUAL_PAGE_SIZE, ORYN_VIRTUAL_FLAG_WRITE | ORYN_VIRTUAL_FLAG_EXECUTE | ORYN_VIRTUAL_FLAG_USER,
            OrynVirtualMmapRegionAnonymous, 0ULL, 0ULL, 0ULL);
    }
    if (ok)
    {
        OrynKernelPageFaultPolicySetProcessContext(&processSpace);
        OrynKernelPageFaultPolicySetDemandAllocator(&processSpace, physicalMemory);
        ok = OrynKernelPageFaultPolicyHandle(&demandFrame, demandAddress) ==
            OrynKernelPageFaultActionRecover;
        OrynKernelPageFaultPolicySetDemandAllocator(0, 0);
        OrynKernelPageFaultPolicySetProcessContext(0);
    }
    if (ok)
    {
        ok = !OrynVirtualMemoryMap(&processSpace, physicalMemory, userAddress, physical,
            ORYN_VIRTUAL_PAGE_SIZE, ORYN_VIRTUAL_FLAG_READ | ORYN_VIRTUAL_FLAG_USER);
    }
    if (ok)
    {
        ok = !OrynVirtualMemoryProtect(&processSpace, userAddress + ORYN_VIRTUAL_PAGE_SIZE,
            ORYN_VIRTUAL_PAGE_SIZE, ORYN_VIRTUAL_FLAG_READ | ORYN_VIRTUAL_FLAG_USER);
    }
    if (ok)
    {
        ok = !OrynVirtualMemoryUnmap(&processSpace, userAddress + ORYN_VIRTUAL_PAGE_SIZE,
            ORYN_VIRTUAL_PAGE_SIZE);
    }
    if (ok)
    {
        ok = !OrynVirtualMemoryMap(&processSpace, physicalMemory, ORYN_VIRTUAL_KERNEL_BASE, physical,
            ORYN_VIRTUAL_PAGE_SIZE, ORYN_VIRTUAL_FLAG_READ | ORYN_VIRTUAL_FLAG_USER);
    }
    if (ok)
    {
        ok = OrynVirtualMemoryUnmap(&processSpace, userAddress, ORYN_VIRTUAL_PAGE_SIZE);
    }
    if (ok)
    {
        ok = processSpace.ApiValidationRuns >= 7ULL &&
            processSpace.ApiValidationFailures >= 3ULL &&
            processSpace.ApiOverwriteRejects >= 1ULL &&
            processSpace.ApiMissingMappingRejects >= 2ULL &&
            processSpace.ApiInvalidRangeRejects >= 1ULL;
    }
    if (ok)
    {
        virtualMemory->ProcessAddressSpacesCreated += 2U;
        virtualMemory->ProcessAddressSpaceValidationRuns +=
            processSpace.ProcessAddressSpaceValidationRuns +
            childSpace.ProcessAddressSpaceValidationRuns;
        virtualMemory->ProcessAddressSpaceValidationFailures +=
            processSpace.ProcessAddressSpaceValidationFailures +
            childSpace.ProcessAddressSpaceValidationFailures;
        virtualMemory->ProcessAddressSpaceKernelHalfEntries +=
            processSpace.ProcessAddressSpaceKernelHalfEntries +
            childSpace.ProcessAddressSpaceKernelHalfEntries;
        virtualMemory->ProcessAddressSpaceUserHalfIsolatedProofs +=
            processSpace.ProcessAddressSpaceUserHalfIsolated +
            childSpace.ProcessAddressSpaceUserHalfIsolated;
        virtualMemory->ProcessAddressSpaceKernelHalfSharedProofs +=
            processSpace.ProcessAddressSpaceKernelHalfShared +
            childSpace.ProcessAddressSpaceKernelHalfShared;
        virtualMemory->ProcessAddressSpaceDistinctPml4Proofs +=
            processSpace.ProcessAddressSpaceDistinctPml4 +
            childSpace.ProcessAddressSpaceDistinctPml4;
        virtualMemory->ApiMappedPages += processSpace.MappedPages;
        virtualMemory->ApiProtectedPages += processSpace.ProtectedPages;
        virtualMemory->ApiUnmappedPages += processSpace.UnmappedPages;
        virtualMemory->DemandAllocatedUserPages += processSpace.DemandAllocatedPages;
        virtualMemory->AnonymousRegionsCreated += processSpace.AnonymousRegionCount;
        virtualMemory->MmapRegionsCreated += processSpace.MmapRegionCount;
        virtualMemory->FileMmapRegionsCreated += processSpace.FileRegionCount;
        virtualMemory->DeviceMmapRegionsCreated += processSpace.DeviceRegionCount;
        virtualMemory->WriteExecutePolicyChecks += processSpace.WriteExecutePolicyChecks;
        virtualMemory->WriteExecuteDeniedCount += processSpace.WriteExecuteDeniedCount;
        virtualMemory->ApiValidationRuns += processSpace.ApiValidationRuns;
        virtualMemory->ApiValidationFailures += processSpace.ApiValidationFailures;
        virtualMemory->ApiInvalidRangeRejects += processSpace.ApiInvalidRangeRejects;
        virtualMemory->ApiOverwriteRejects += processSpace.ApiOverwriteRejects;
        virtualMemory->ApiMissingMappingRejects += processSpace.ApiMissingMappingRejects;
        virtualMemory->ApiPartialRollbackPages += processSpace.ApiPartialRollbackPages;
        virtualMemory->UserCopyBytesIn += sizeof(kernelCopy);
        virtualMemory->UserCopyBytesOut += sizeof(userSeed);
        virtualMemory->CopyOnWriteCloneCount += 1ULL;
        virtualMemory->CopyOnWriteSharedPages += processSpace.CopyOnWriteSharedPages + childSpace.CopyOnWriteSharedPages;
        virtualMemory->CopyOnWriteResolvedPages += childSpace.CopyOnWriteResolvedPages;
    }
    return ok;
}
