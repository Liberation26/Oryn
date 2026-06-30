#include "BootX64Internal.h"

#define ORYN_PAGE_PRESENT 0x001ULL
#define ORYN_PAGE_WRITABLE 0x002ULL
#define ORYN_PAGE_SIZE_2MB 0x080ULL
#define ORYN_PAGE_FLAGS (ORYN_PAGE_PRESENT | ORYN_PAGE_WRITABLE)
#define ORYN_PAGE_FLAGS_2MB (ORYN_PAGE_FLAGS | ORYN_PAGE_SIZE_2MB)
#define ORYN_PAGE_ADDRESS_MASK 0x000FFFFFFFFFF000ULL
#define ORYN_2MB_PAGE_MASK 0x000FFFFFFFE00000ULL
#define ORYN_2MB_SIZE 0x200000ULL
#define ORYN_IDENTITY_MAP_BYTES 0x1000000000ULL

static UINT64* gKernelPml4;
static UINT64 gKernelPml4Physical;

static UINT64 AlignDown4K(UINT64 value)
{
    return value & ~(PAGE_SIZE - 1ULL);
}

static UINT64 AlignUp4K(UINT64 value)
{
    return (value + PAGE_SIZE - 1ULL) & ~(PAGE_SIZE - 1ULL);
}

static EFI_STATUS AllocatePageTable(UINT64** outTable)
{
    EFI_PHYSICAL_ADDRESS address = ORYN_IDENTITY_MAP_BYTES - 1ULL;
    EFI_STATUS status = gBootServices->AllocatePages(AllocateMaxAddress, EfiLoaderData, 1, &address);
    if (IsError(status))
    {
        return status;
    }

    SetMemory((void*)(UINTN)address, 0, (UINTN)PAGE_SIZE);
    *outTable = (UINT64*)(UINTN)address;
    return EFI_SUCCESS;
}

static EFI_STATUS GetOrCreateTable(UINT64* table, UINTN index, UINT64** outNext)
{
    if ((table[index] & ORYN_PAGE_PRESENT) != 0ULL)
    {
        *outNext = (UINT64*)(UINTN)(table[index] & ORYN_PAGE_ADDRESS_MASK);
        return EFI_SUCCESS;
    }

    UINT64* next = ORYN_NULL;
    EFI_STATUS status = AllocatePageTable(&next);
    if (IsError(status))
    {
        return status;
    }

    table[index] = ((UINT64)(UINTN)next & ORYN_PAGE_ADDRESS_MASK) | ORYN_PAGE_FLAGS;
    *outNext = next;
    return EFI_SUCCESS;
}

static EFI_STATUS MapPage4K(UINT64* pml4, UINT64 virtualAddress, UINT64 physicalAddress)
{
    UINTN pml4Index = (UINTN)((virtualAddress >> 39) & 0x1FFULL);
    UINTN pdptIndex = (UINTN)((virtualAddress >> 30) & 0x1FFULL);
    UINTN pdIndex = (UINTN)((virtualAddress >> 21) & 0x1FFULL);
    UINTN ptIndex = (UINTN)((virtualAddress >> 12) & 0x1FFULL);

    UINT64* pdpt = ORYN_NULL;
    UINT64* pd = ORYN_NULL;
    UINT64* pt = ORYN_NULL;
    EFI_STATUS status = GetOrCreateTable(pml4, pml4Index, &pdpt);
    if (IsError(status))
    {
        return status;
    }

    status = GetOrCreateTable(pdpt, pdptIndex, &pd);
    if (IsError(status))
    {
        return status;
    }

    status = GetOrCreateTable(pd, pdIndex, &pt);
    if (IsError(status))
    {
        return status;
    }

    pt[ptIndex] = (physicalAddress & ORYN_PAGE_ADDRESS_MASK) | ORYN_PAGE_FLAGS;
    return EFI_SUCCESS;
}

static EFI_STATUS MapPage2MB(UINT64* pml4, UINT64 virtualAddress, UINT64 physicalAddress)
{
    UINTN pml4Index = (UINTN)((virtualAddress >> 39) & 0x1FFULL);
    UINTN pdptIndex = (UINTN)((virtualAddress >> 30) & 0x1FFULL);
    UINTN pdIndex = (UINTN)((virtualAddress >> 21) & 0x1FFULL);

    UINT64* pdpt = ORYN_NULL;
    UINT64* pd = ORYN_NULL;
    EFI_STATUS status = GetOrCreateTable(pml4, pml4Index, &pdpt);
    if (IsError(status))
    {
        return status;
    }

    status = GetOrCreateTable(pdpt, pdptIndex, &pd);
    if (IsError(status))
    {
        return status;
    }

    pd[pdIndex] = (physicalAddress & ORYN_2MB_PAGE_MASK) | ORYN_PAGE_FLAGS_2MB;
    return EFI_SUCCESS;
}

static EFI_STATUS MapIdentity2MB(UINT64* pml4, UINT64 bytes)
{
    for (UINT64 address = 0; address < bytes; address += ORYN_2MB_SIZE)
    {
        EFI_STATUS status = MapPage2MB(pml4, address, address);
        if (IsError(status))
        {
            return status;
        }
    }

    return EFI_SUCCESS;
}

static EFI_STATUS MapKernelVirtual4K(UINT64* pml4, const OrynKernelElfLayout* layout)
{
    UINT64 firstVirtual = AlignDown4K(layout->VirtualBase);
    UINT64 firstPhysical = AlignDown4K(layout->PhysicalBase);
    UINT64 endVirtual = AlignUp4K(layout->VirtualBase + layout->VirtualSize);

    for (UINT64 virtualAddress = firstVirtual; virtualAddress < endVirtual; virtualAddress += PAGE_SIZE)
    {
        UINT64 physicalAddress = firstPhysical + (virtualAddress - firstVirtual);
        EFI_STATUS status = MapPage4K(pml4, virtualAddress, physicalAddress);
        if (IsError(status))
        {
            return status;
        }
    }

    return EFI_SUCCESS;
}

EFI_STATUS PrepareKernelVirtualAddressSpace(const OrynKernelElfLayout* layout)
{
    if (layout == ORYN_NULL || layout->VirtualBase == 0ULL || layout->VirtualSize == 0ULL)
    {
        Print("[BOOT] FAIL: Kernel virtual layout is missing.\n");
        return EFI_INVALID_PARAMETER;
    }

    EFI_STATUS status = AllocatePageTable(&gKernelPml4);
    if (IsError(status))
    {
        Print("[BOOT] FAIL: Could not allocate kernel PML4.\n");
        return status;
    }

    gKernelPml4Physical = (UINT64)(UINTN)gKernelPml4;
    status = MapIdentity2MB(gKernelPml4, ORYN_IDENTITY_MAP_BYTES);
    if (IsError(status))
    {
        Print("[BOOT] FAIL: Could not identity-map the early low address space.\n");
        return status;
    }

    status = MapKernelVirtual4K(gKernelPml4, layout);
    if (IsError(status))
    {
        Print("[BOOT] FAIL: Could not map the chosen kernel virtual layout.\n");
        return status;
    }

    Print("[BOOT] Temporary identity map bytes: ");
    PrintHex64(ORYN_IDENTITY_MAP_BYTES);
    Print("\n");
    Print("[BOOT] PASS: Temporary higher-half/chosen virtual map prepared. PML4 ");
    PrintHex64(gKernelPml4Physical);
    Print("\n");
    Print("[BOOT] Kernel virtual map base: ");
    PrintHex64(layout->VirtualBase);
    Print("\n");
    Print("[BOOT] Kernel virtual map size: ");
    PrintHex64(layout->VirtualSize);
    Print(" bytes.\n");
    return EFI_SUCCESS;
}

void ActivateKernelVirtualAddressSpace(void)
{
    if (gKernelPml4Physical == 0ULL)
    {
        return;
    }

    __asm__ volatile ("mov %0, %%cr3" :: "r"(gKernelPml4Physical) : "memory");
    Print("[BOOT] PASS: Temporary higher-half/chosen virtual map activated.\n");
}
