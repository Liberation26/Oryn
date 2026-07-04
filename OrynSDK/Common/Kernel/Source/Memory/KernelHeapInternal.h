#ifndef ORYN_KERNEL_HEAP_INTERNAL_H
#define ORYN_KERNEL_HEAP_INTERNAL_H

#include "KernelHeap.h"
#include "KernelDiagnosticsLogger.h"
#include "KernelScreenReport.h"
#include "string.h"

#define ORYN_KERNEL_HEAP_MAGIC 0x4F52484D414C4C4FULL
#define ORYN_KERNEL_HEAP_FREE_MAGIC 0x4F5248465245454FULL
#define ORYN_KERNEL_SLAB_MAGIC 0x4F52534C41424C4FULL
#define ORYN_KERNEL_HEAP_MIN_SPLIT 64ULL
#define ORYN_KERNEL_HEAP_FLAG_FREE 0x00000001U
#define ORYN_KERNEL_HEAP_FLAG_SLAB 0x00000002U
#define ORYN_KERNEL_HEAP_FLAG_CRITICAL 0x00000004U
#define ORYN_KERNEL_HEAP_FLAG_OBJECT_CACHE 0x00000008U
#define ORYN_KERNEL_HEAP_ALIGN 16ULL
#define ORYN_KERNEL_HEAP_NO_OBJECT_CACHE 0xFFFFFFFFU

typedef struct OrynKernelHeapBlock
{
    unsigned long long Magic;
    unsigned long long Size;
    unsigned long long RequestedSize;
    unsigned int Flags;
    unsigned int SlabCacheIndex;
    unsigned int ObjectCacheIndex;
    struct OrynKernelHeapBlock* Previous;
    struct OrynKernelHeapBlock* Next;
} OrynKernelHeapBlock;

typedef struct OrynKernelSlabFreeObject
{
    struct OrynKernelSlabFreeObject* Next;
} OrynKernelSlabFreeObject;

typedef struct OrynKernelSlabCache
{
    unsigned long long ObjectSize;
    OrynKernelSlabFreeObject* FreeList;
    OrynKernelSlabCacheStats Stats;
} OrynKernelSlabCache;

extern OrynKernelPhysicalMemory* gOrynHeapPhysicalMemory;
extern OrynKernelVirtualMemory* gOrynHeapVirtualMemory;
extern OrynKernelHeapBlock* gOrynHeapHead;
extern OrynKernelHeapStats gOrynHeapStats;
extern OrynKernelSlabCache gOrynSlabCaches[ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT];
extern OrynKernelObjectCacheStats gOrynObjectCaches[ORYN_KERNEL_HEAP_OBJECT_CACHE_COUNT];

unsigned long long OrynHeapAlignUp(unsigned long long value);
void OrynHeapClearMemory(void* pointer, unsigned long long size);
void OrynHeapCopyMemory(void* target, const void* source, unsigned long long size);
OrynKernelHeapBlock* OrynHeapPointerToBlock(void* pointer);
void* OrynHeapBlockToPointer(OrynKernelHeapBlock* block);
void OrynHeapAddFreeBytes(unsigned long long bytes);
void OrynHeapRemoveFreeBytes(unsigned long long bytes);
void OrynHeapAddAllocatedBytes(unsigned long long bytes);
void OrynHeapRemoveAllocatedBytes(unsigned long long bytes);
void OrynHeapAddRequestedBytes(unsigned long long bytes);
void OrynHeapRemoveRequestedBytes(unsigned long long bytes);
void OrynHeapRefreshLeakCounters(void);
unsigned long long OrynHeapAllocatePage(void);
void* OrynHeapAllocateRaw(unsigned long long size, unsigned int flags);
void OrynHeapFreeRaw(void* pointer);
unsigned int OrynHeapFindSlabCache(unsigned long long size);
void* OrynHeapSlabAllocate(unsigned int cacheIndex, unsigned long long requested);
int OrynHeapSlabFree(void* pointer);
int OrynHeapValidateObjectCaches(void);
int OrynHeapRunObjectCacheSelfTest(void);

#endif
