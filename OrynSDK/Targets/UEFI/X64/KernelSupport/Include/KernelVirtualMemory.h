#ifndef ORYN_KERNEL_VIRTUAL_MEMORY_H
#define ORYN_KERNEL_VIRTUAL_MEMORY_H

#include "KernelMemoryMap.h"
#include "KernelPhysicalMemory.h"
#include "OrynBootInfo.h"

#define ORYN_VIRTUAL_PAGE_SIZE 4096ULL
#define ORYN_VIRTUAL_TABLE_ENTRY_COUNT 512U
#define ORYN_VIRTUAL_TABLE_FLAGS_PRESENT 0x001ULL
#define ORYN_VIRTUAL_TABLE_FLAGS_WRITABLE 0x002ULL
#define ORYN_VIRTUAL_TABLE_FLAGS_DEFAULT \
    (ORYN_VIRTUAL_TABLE_FLAGS_PRESENT | ORYN_VIRTUAL_TABLE_FLAGS_WRITABLE)

#define ORYN_VIRTUAL_FLAG_READ 0x001ULL
#define ORYN_VIRTUAL_FLAG_WRITE 0x002ULL
#define ORYN_VIRTUAL_FLAG_EXECUTE 0x004ULL
#define ORYN_VIRTUAL_FLAG_USER 0x008ULL
#define ORYN_VIRTUAL_FLAG_GUARD 0x010ULL
#define ORYN_VIRTUAL_FLAG_GLOBAL 0x020ULL
#define ORYN_VIRTUAL_FLAG_COPY_ON_WRITE 0x040ULL
#define ORYN_VIRTUAL_USER_BASE 0x0000000000400000ULL
#define ORYN_VIRTUAL_USER_LIMIT 0x0000800000000000ULL
#define ORYN_VIRTUAL_KERNEL_BASE 0xFFFF800000000000ULL
#define ORYN_VIRTUAL_KERNEL_LIMIT 0xFFFFFFFFFFFFFFFFULL
#define ORYN_VIRTUAL_MAX_ANONYMOUS_REGIONS 32U
#define ORYN_VIRTUAL_MAX_MMAP_REGIONS ORYN_VIRTUAL_MAX_ANONYMOUS_REGIONS
#define ORYN_VIRTUAL_MMAP_SOURCE_NONE 0ULL
#define ORYN_USER_COPY_OK 1
#define ORYN_USER_COPY_FAIL 0

typedef enum OrynVirtualMmapRegionType
{
    OrynVirtualMmapRegionUnused = 0,
    OrynVirtualMmapRegionAnonymous = 1,
    OrynVirtualMmapRegionFile = 2,
    OrynVirtualMmapRegionDevice = 3
} OrynVirtualMmapRegionType;

typedef struct OrynVirtualAnonymousRegion
{
    unsigned int Used;
    unsigned long long Base;
    unsigned long long Bytes;
    unsigned long long Flags;
    unsigned long long CommittedPages;
    unsigned int CopyOnWriteInherited;
    unsigned int Type;
    unsigned long long SourceId;
    unsigned long long SourceOffset;
    unsigned long long DevicePhysical;
    unsigned long long DeviceBytes;
} OrynVirtualAnonymousRegion;

typedef struct OrynKernelAddressSpace
{
    unsigned int Initialized;
    unsigned int ProcessOwned;
    unsigned int AddressSpaceId;
    unsigned long long Pml4Physical;
    unsigned long long UserBase;
    unsigned long long UserLimit;
    unsigned long long KernelBase;
    unsigned long long KernelLimit;
    unsigned long long MappedPages;
    unsigned long long ProtectedPages;
    unsigned long long UnmappedPages;
    unsigned long long DemandAllocatedPages;
    unsigned long long CopyOnWriteSharedPages;
    unsigned long long CopyOnWriteResolvedPages;
    unsigned long long CopyOnWriteClonePages;
    unsigned long long AnonymousRegionCount;
    unsigned long long MmapRegionCount;
    unsigned long long FileRegionCount;
    unsigned long long DeviceRegionCount;
    unsigned long long WriteExecuteDeniedCount;
    unsigned long long WriteExecutePolicyChecks;
    unsigned long long ApiValidationRuns;
    unsigned long long ApiValidationFailures;
    unsigned long long ApiInvalidRangeRejects;
    unsigned long long ApiOverwriteRejects;
    unsigned long long ApiMissingMappingRejects;
    unsigned long long ApiPartialRollbackPages;
    OrynVirtualAnonymousRegion AnonymousRegions[ORYN_VIRTUAL_MAX_ANONYMOUS_REGIONS];
} OrynKernelAddressSpace;

typedef struct OrynKernelVirtualMemory
{
    unsigned int Initialized;
    unsigned int Active;
    unsigned int FramebufferSelected;
    unsigned int FramebufferMapped;
    unsigned int DefaultScreenMapped;
    unsigned int VgaTextMapped;
    unsigned int FontMapped;
    unsigned int MapFailure;
    unsigned int TablesAllocated;
    unsigned long long CurrentCr3;
    unsigned long long CurrentStackPointer;
    unsigned long long NewPml4;
    unsigned long long KernelMapStart;
    unsigned long long KernelMapEnd;
    unsigned long long KernelVirtualMapStart;
    unsigned long long KernelVirtualMapEnd;
    unsigned long long KernelEntryPhysical;
    unsigned long long KernelEntryVirtual;
    unsigned long long BootInfoMapStart;
    unsigned long long BootInfoMapEnd;
    unsigned long long MemoryMapMapStart;
    unsigned long long MemoryMapMapEnd;
    unsigned long long StackMapStart;
    unsigned long long StackMapEnd;
    unsigned long long FramebufferMapStart;
    unsigned long long FramebufferMapEnd;
    unsigned long long DefaultScreenMapStart;
    unsigned long long DefaultScreenMapEnd;
    unsigned long long VgaTextMapStart;
    unsigned long long VgaTextMapEnd;
    unsigned long long FontMapStart;
    unsigned long long FontMapEnd;
    unsigned long long IdentityMappedPages;
    unsigned long long KernelVirtualMappedPages;
    unsigned long long UserBase;
    unsigned long long UserLimit;
    unsigned long long KernelBase;
    unsigned long long KernelLimit;
    unsigned long long ApiMappedPages;
    unsigned long long ApiUnmappedPages;
    unsigned long long ApiProtectedPages;
    unsigned int AddressSpaceApiReady;
    unsigned int PageFaultPolicyReady;
    unsigned int ProcessAddressSpacesCreated;
    unsigned long long DemandAllocatedUserPages;
    unsigned long long AnonymousRegionsCreated;
    unsigned long long UserCopyBytesIn;
    unsigned long long UserCopyBytesOut;
    unsigned long long UserCopyFaults;
    unsigned long long CopyOnWriteCloneCount;
    unsigned long long CopyOnWriteSharedPages;
    unsigned long long CopyOnWriteResolvedPages;
    unsigned long long MmapRegionsCreated;
    unsigned long long FileMmapRegionsCreated;
    unsigned long long DeviceMmapRegionsCreated;
    unsigned long long WriteExecutePolicyChecks;
    unsigned long long WriteExecuteDeniedCount;
    unsigned long long ApiValidationRuns;
    unsigned long long ApiValidationFailures;
    unsigned long long ApiInvalidRangeRejects;
    unsigned long long ApiOverwriteRejects;
    unsigned long long ApiMissingMappingRejects;
    unsigned long long ApiPartialRollbackPages;
    unsigned int HigherHalfReady;
    unsigned int HigherHalfCanonical;
    unsigned int HigherHalfAligned;
    unsigned int HigherHalfNoUserOverlap;
    unsigned int HigherHalfEntryMapped;
    unsigned int HigherHalfPml4SlotReady;
    unsigned int HigherHalfPhysicalWindowValid;
    unsigned long long HigherHalfMappedPageProofCount;
    unsigned long long HigherHalfValidationFailures;
    OrynKernelAddressSpace KernelAddressSpace;
} OrynKernelVirtualMemory;

unsigned long long OrynVirtualMemoryReadCr3(void);
int OrynVirtualMemoryIsUserAddress(unsigned long long virtualAddress);
int OrynVirtualMemoryIsKernelAddress(unsigned long long virtualAddress);
int OrynVirtualMemoryInitKernelAddressSpace(OrynKernelVirtualMemory* virtualMemory);
int OrynVirtualMemoryCreateProcessAddressSpace(
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelAddressSpace* addressSpace);
int OrynVirtualMemoryMap(
    OrynKernelAddressSpace* addressSpace,
    OrynKernelPhysicalMemory* physicalMemory,
    unsigned long long virtualAddress,
    unsigned long long physicalAddress,
    unsigned long long bytes,
    unsigned long long flags);
int OrynVirtualMemoryUnmap(
    OrynKernelAddressSpace* addressSpace,
    unsigned long long virtualAddress,
    unsigned long long bytes);
int OrynVirtualMemoryProtect(
    OrynKernelAddressSpace* addressSpace,
    unsigned long long virtualAddress,
    unsigned long long bytes,
    unsigned long long flags);
int OrynVirtualMemoryCreateCopyOnWriteClone(
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelAddressSpace* parentAddressSpace,
    OrynKernelAddressSpace* childAddressSpace);
int OrynVirtualMemoryResolveCopyOnWriteFault(
    OrynKernelAddressSpace* addressSpace,
    OrynKernelPhysicalMemory* physicalMemory,
    unsigned long long faultAddress);

int OrynVirtualMemoryFlagsRespectWriteXorExecute(unsigned long long flags);
int OrynVirtualMemoryReserveMmapRegion(
    OrynKernelAddressSpace* addressSpace,
    unsigned long long virtualAddress,
    unsigned long long bytes,
    unsigned long long flags,
    unsigned int type,
    unsigned long long sourceId,
    unsigned long long sourceOffset,
    unsigned long long devicePhysical);
int OrynVirtualMemoryReserveAnonymousRegion(
    OrynKernelAddressSpace* addressSpace,
    unsigned long long virtualAddress,
    unsigned long long bytes,
    unsigned long long flags);
int OrynVirtualMemoryDemandAllocateUserPage(
    OrynKernelAddressSpace* addressSpace,
    OrynKernelPhysicalMemory* physicalMemory,
    unsigned long long faultAddress,
    unsigned long long flags);
int OrynVirtualMemoryIsRangeInUserSpace(
    unsigned long long virtualAddress,
    unsigned long long bytes);
int OrynVirtualMemoryIsUserRangeMapped(
    OrynKernelAddressSpace* addressSpace,
    unsigned long long virtualAddress,
    unsigned long long bytes,
    unsigned long long requiredFlags);
int OrynCopyFromUser(
    OrynKernelAddressSpace* addressSpace,
    void* kernelDestination,
    const void* userSource,
    unsigned long long bytes);
int OrynCopyToUser(
    OrynKernelAddressSpace* addressSpace,
    void* userDestination,
    const void* kernelSource,
    unsigned long long bytes);
int OrynVirtualMemoryRunAddressSpaceSelfTest(
    OrynKernelVirtualMemory* virtualMemory,
    OrynKernelPhysicalMemory* physicalMemory);
int OrynVirtualMemoryUnmapGuardPage(unsigned long long virtualAddress);
int OrynVirtualMemoryValidateHigherHalfKernelMap(OrynKernelVirtualMemory* virtualMemory);
int OrynVirtualMemoryInit(
    const OrynBootInfo* bootInfo,
    const OrynKernelMemoryMap* memoryMap,
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelVirtualMemory* virtualMemory);
void OrynVirtualMemoryPrintProof(const OrynKernelVirtualMemory* virtualMemory);

#endif
