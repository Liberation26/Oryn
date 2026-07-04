#include "KernelVirtualMemory.h"

#define ORYN_ADDRESS_SPACE_PAGE_MASK 0x000FFFFFFFFFF000ULL
#define ORYN_ADDRESS_SPACE_PRESENT 0x001ULL

static unsigned long long* AddressSpacePml4(const OrynKernelAddressSpace* addressSpace)
{
    if (addressSpace == 0 || addressSpace->Pml4Physical == 0ULL)
    {
        return 0;
    }
    return (unsigned long long*)(addressSpace->Pml4Physical & ORYN_ADDRESS_SPACE_PAGE_MASK);
}

static unsigned int CountPresentEntries(
    const unsigned long long* pml4,
    unsigned int first,
    unsigned int end)
{
    unsigned int count = 0U;
    if (pml4 == 0)
    {
        return 0U;
    }
    for (unsigned int index = first; index < end; ++index)
    {
        if ((pml4[index] & ORYN_ADDRESS_SPACE_PRESENT) != 0ULL)
        {
            count += 1U;
        }
    }
    return count;
}

static int UserHalfIsIsolated(
    const unsigned long long* processPml4,
    const unsigned long long* kernelPml4)
{
    if (processPml4 == 0 || kernelPml4 == 0)
    {
        return 0;
    }
    for (unsigned int index = 0U; index < 256U; ++index)
    {
        if ((processPml4[index] & ORYN_ADDRESS_SPACE_PRESENT) != 0ULL &&
            (kernelPml4[index] & ORYN_ADDRESS_SPACE_PRESENT) != 0ULL &&
            ((processPml4[index] ^ kernelPml4[index]) & ORYN_ADDRESS_SPACE_PAGE_MASK) == 0ULL)
        {
            return 0;
        }
    }
    return 1;
}

static int KernelHalfMatches(
    const unsigned long long* processPml4,
    const unsigned long long* kernelPml4,
    unsigned long long* sharedEntries)
{
    unsigned long long count = 0ULL;
    if (processPml4 == 0 || kernelPml4 == 0)
    {
        return 0;
    }
    for (unsigned int index = 256U; index < 512U; ++index)
    {
        if ((kernelPml4[index] & ORYN_ADDRESS_SPACE_PRESENT) != 0ULL)
        {
            if (processPml4[index] != kernelPml4[index])
            {
                return 0;
            }
            count += 1ULL;
        }
    }
    if (sharedEntries != 0)
    {
        *sharedEntries = count;
    }
    return count != 0ULL;
}

int OrynVirtualMemoryValidateProcessAddressSpace(
    OrynKernelAddressSpace* addressSpace,
    const OrynKernelAddressSpace* kernelAddressSpace)
{
    unsigned long long* processPml4;
    unsigned long long* kernelPml4;
    unsigned long long sharedEntries = 0ULL;
    int ok;

    if (addressSpace != 0)
    {
        addressSpace->ProcessAddressSpaceValidationRuns += 1ULL;
    }

    processPml4 = AddressSpacePml4(addressSpace);
    kernelPml4 = AddressSpacePml4(kernelAddressSpace);
    ok = addressSpace != 0 && kernelAddressSpace != 0 &&
        addressSpace->Initialized != 0U && addressSpace->ProcessOwned != 0U &&
        kernelAddressSpace->Initialized != 0U && kernelAddressSpace->ProcessOwned == 0U &&
        addressSpace->AddressSpaceId != 0U &&
        processPml4 != 0 && kernelPml4 != 0 && processPml4 != kernelPml4 &&
        addressSpace->UserBase == ORYN_VIRTUAL_USER_BASE &&
        addressSpace->UserLimit == ORYN_VIRTUAL_USER_LIMIT &&
        addressSpace->KernelBase == ORYN_VIRTUAL_KERNEL_BASE &&
        addressSpace->KernelLimit == ORYN_VIRTUAL_KERNEL_LIMIT &&
        UserHalfIsIsolated(processPml4, kernelPml4) &&
        KernelHalfMatches(processPml4, kernelPml4, &sharedEntries);

    if (!ok)
    {
        if (addressSpace != 0)
        {
            addressSpace->ProcessAddressSpaceValidationFailures += 1ULL;
        }
        return 0;
    }

    addressSpace->ProcessAddressSpaceKernelHalfEntries = sharedEntries;
    addressSpace->ProcessAddressSpaceUserHalfIsolated = 1U;
    addressSpace->ProcessAddressSpaceKernelHalfShared = 1U;
    addressSpace->ProcessAddressSpaceDistinctPml4 = 1U;
    return 1;
}
