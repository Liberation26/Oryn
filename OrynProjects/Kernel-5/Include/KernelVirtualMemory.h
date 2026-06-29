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
    unsigned long long NewPml4;
    unsigned long long KernelMapStart;
    unsigned long long KernelMapEnd;
    unsigned long long BootInfoMapStart;
    unsigned long long BootInfoMapEnd;
    unsigned long long MemoryMapMapStart;
    unsigned long long MemoryMapMapEnd;
    unsigned long long FramebufferMapStart;
    unsigned long long FramebufferMapEnd;
    unsigned long long DefaultScreenMapStart;
    unsigned long long DefaultScreenMapEnd;
    unsigned long long VgaTextMapStart;
    unsigned long long VgaTextMapEnd;
    unsigned long long FontMapStart;
    unsigned long long FontMapEnd;
    unsigned long long IdentityMappedPages;
} OrynKernelVirtualMemory;

unsigned long long OrynVirtualMemoryReadCr3(void);
int OrynVirtualMemoryInit(
    const OrynBootInfo* bootInfo,
    const OrynKernelMemoryMap* memoryMap,
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelVirtualMemory* virtualMemory);
void OrynVirtualMemoryPrintProof(const OrynKernelVirtualMemory* virtualMemory);

#endif
