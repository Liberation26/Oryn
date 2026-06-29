#include "KernelPhysicalMemory.h"
#include "KernelIo.h"

static void WritePageCount(const char* label, unsigned int pages)
{
    unsigned long long bytes = ((unsigned long long)pages * ORYN_PHYSICAL_PAGE_SIZE);

    KernelIoWriteString(label);
    KernelIoWriteDec64(pages);
    KernelIoWriteString(" pages, ");
    KernelIoWriteDec64(bytes / 1024ULL);
    KernelIoWriteString(" KB, ");
    KernelIoWriteDec64(bytes / (1024ULL * 1024ULL));
    KernelIoWriteString(" MB\n");
}

void OrynPhysicalMemoryPrintSummary(const OrynKernelPhysicalMemory* allocator)
{
    if (allocator == 0 || allocator->Initialized == 0U)
    {
        KernelIoWriteString("[KERNEL] Physical memory allocator: unavailable\n");
        return;
    }

    KernelIoWriteString("[KERNEL] Physical memory allocator: available\n");
    KernelIoWriteString("[KERNEL] Page size: ");
    KernelIoWriteDec64(allocator->PageSize);
    KernelIoWriteString("\n");

    WritePageCount("[KERNEL] Total usable pages: ", allocator->TotalUsablePages);
    WritePageCount("[KERNEL] Tracked free pages: ", allocator->FreePageCount);
    WritePageCount("[KERNEL] Used pages: ", allocator->UsedPageCount);
    WritePageCount("[KERNEL] Low pages held back: ", allocator->ReservedLowPages);
    WritePageCount("[KERNEL] Untracked usable pages: ", allocator->UntrackedUsablePages);

    KernelIoWriteString("[KERNEL] Lowest tracked free page: ");
    KernelIoWriteHex64(allocator->LowestFreeAddress);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] Highest tracked free page: ");
    KernelIoWriteHex64(allocator->HighestFreeAddress);
    KernelIoWriteString("\n");

    if (allocator->UntrackedUsablePages != 0U)
    {
        KernelIoWriteString("[KERNEL] WARN: Physical allocator static capacity was reached.\n");
    }
}

void OrynPhysicalMemoryPrintFinalState(const OrynKernelPhysicalMemory* allocator)
{
    if (allocator == 0 || allocator->Initialized == 0U)
    {
        KernelIoWriteString("[KERNEL] Physical memory allocator final state: unavailable\n");
        return;
    }

    KernelIoWriteString("[KERNEL] Physical memory allocator final state:\n");
    WritePageCount("  Tracked free pages: ", allocator->FreePageCount);
    WritePageCount("  Used pages: ", allocator->UsedPageCount);
    KernelIoWriteString("  Lowest tracked free page: ");
    KernelIoWriteHex64(allocator->LowestFreeAddress);
    KernelIoWriteString("\n");
    KernelIoWriteString("  Highest tracked free page: ");
    KernelIoWriteHex64(allocator->HighestFreeAddress);
    KernelIoWriteString("\n");
}

void OrynPhysicalMemoryRunSelfTest(OrynKernelPhysicalMemory* allocator)
{
    unsigned long long page1;
    unsigned long long page2;
    unsigned long long page3;
    int freed;

    if (allocator == 0 || allocator->Initialized == 0U)
    {
        KernelIoWriteString("[KERNEL] Physical allocator self-test: skipped\n");
        return;
    }

    page1 = OrynPhysicalMemoryAllocatePage(allocator);
    page2 = OrynPhysicalMemoryAllocatePage(allocator);

    KernelIoWriteString("[KERNEL] Test alloc page 1: ");
    KernelIoWriteHex64(page1);
    KernelIoWriteString("\n");

    KernelIoWriteString("[KERNEL] Test alloc page 2: ");
    KernelIoWriteHex64(page2);
    KernelIoWriteString("\n");

    freed = OrynPhysicalMemoryFreePage(allocator, page1);
    KernelIoWriteString("[KERNEL] Test free page 1: ");
    KernelIoWriteString(freed ? "ok" : "fail");
    KernelIoWriteString("\n");

    page3 = OrynPhysicalMemoryAllocatePage(allocator);
    KernelIoWriteString("[KERNEL] Test alloc page 3: ");
    KernelIoWriteHex64(page3);
    KernelIoWriteString("\n");

    if (page2 != ORYN_PHYSICAL_ALLOC_FAIL)
    {
        (void)OrynPhysicalMemoryFreePage(allocator, page2);
    }

    if (page3 != ORYN_PHYSICAL_ALLOC_FAIL)
    {
        (void)OrynPhysicalMemoryFreePage(allocator, page3);
    }

    KernelIoWriteString("[KERNEL] Physical allocator self-test: complete\n");
}
