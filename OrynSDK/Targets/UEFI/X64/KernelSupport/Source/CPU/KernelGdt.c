#include "KernelGdt.h"
#include "KernelIo.h"

#define ORYN_GDT_ASSERT(name, condition) typedef char name[(condition) ? 1 : -1]
#define ORYN_GDT_ACCESS_CODE 0x9AU
#define ORYN_GDT_ACCESS_DATA 0x92U
#define ORYN_GDT_ACCESS_USER_CODE 0xFAU
#define ORYN_GDT_ACCESS_USER_DATA 0xF2U
#define ORYN_GDT_ACCESS_TSS 0x89U
#define ORYN_GDT_FLAGS_CODE 0x0AU
#define ORYN_GDT_FLAGS_DATA 0x0CU
#define ORYN_GDT_SEGMENT_LIMIT 0xFFFFFU

typedef struct OrynGdtPointer
{
    unsigned short Limit;
    unsigned long long Base;
} __attribute__((packed)) OrynGdtPointer;

typedef struct OrynTss
{
    unsigned int Reserved0;
    unsigned long long Rsp0;
    unsigned long long Rsp1;
    unsigned long long Rsp2;
    unsigned long long Reserved1;
    unsigned long long Ist1;
    unsigned long long Ist2;
    unsigned long long Ist3;
    unsigned long long Ist4;
    unsigned long long Ist5;
    unsigned long long Ist6;
    unsigned long long Ist7;
    unsigned long long Reserved2;
    unsigned short Reserved3;
    unsigned short IoMapBase;
} __attribute__((packed)) OrynTss;

ORYN_GDT_ASSERT(OrynGdtPointerSizeMustBe10, sizeof(OrynGdtPointer) == 10U);
ORYN_GDT_ASSERT(OrynTssSizeMustBe104, sizeof(OrynTss) == 104U);

static unsigned long long gGdt[ORYN_GDT_ENTRY_COUNT] __attribute__((aligned(16)));
static OrynTss gTss __attribute__((aligned(16)));
static unsigned char gExceptionIstStack[ORYN_GDT_IST_STACK_SIZE] __attribute__((aligned(16)));
static OrynKernelGdtState gGdtState;

static void ClearBytes(void* target, unsigned long long count)
{
    unsigned char* bytes = (unsigned char*)target;
    for (unsigned long long index = 0; index < count; ++index)
    {
        bytes[index] = 0U;
    }
}

static unsigned long long MakeSegmentDescriptor(
    unsigned int base,
    unsigned int limit,
    unsigned char access,
    unsigned char flags)
{
    unsigned long long descriptor = 0ULL;
    descriptor |= (unsigned long long)(limit & 0xFFFFU);
    descriptor |= (unsigned long long)(base & 0xFFFFU) << 16;
    descriptor |= (unsigned long long)((base >> 16) & 0xFFU) << 32;
    descriptor |= (unsigned long long)access << 40;
    descriptor |= (unsigned long long)((limit >> 16) & 0x0FU) << 48;
    descriptor |= (unsigned long long)(flags & 0x0FU) << 52;
    descriptor |= (unsigned long long)((base >> 24) & 0xFFU) << 56;
    return descriptor;
}

static void SetTssDescriptor(unsigned int index, unsigned long long base, unsigned int limit)
{
    unsigned long long low = 0ULL;
    unsigned long long high = 0ULL;
    low |= (unsigned long long)(limit & 0xFFFFU);
    low |= (base & 0xFFFFFFULL) << 16;
    low |= (unsigned long long)ORYN_GDT_ACCESS_TSS << 40;
    low |= (unsigned long long)((limit >> 16) & 0x0FU) << 48;
    low |= ((base >> 24) & 0xFFULL) << 56;
    high = (base >> 32) & 0xFFFFFFFFULL;
    gGdt[index] = low;
    gGdt[index + 1U] = high;
}

static void StoreGdt(OrynGdtPointer* pointer)
{
    __asm__ volatile ("sgdt %0" : "=m"(*pointer));
}

static unsigned short ReadCs(void)
{
    unsigned short value;
    __asm__ volatile ("mov %%cs, %0" : "=r"(value));
    return value;
}

static unsigned short ReadDs(void)
{
    unsigned short value;
    __asm__ volatile ("mov %%ds, %0" : "=r"(value));
    return value;
}

static unsigned short ReadTr(void)
{
    unsigned short value;
    __asm__ volatile ("str %0" : "=r"(value));
    return value;
}

static void LoadTaskRegister(unsigned short selector)
{
    __asm__ volatile ("ltr %0" :: "r"(selector) : "memory");
}

static void LoadGdtAndSegments(
    const OrynGdtPointer* pointer,
    unsigned short codeSelector,
    unsigned short dataSelector)
{
    unsigned long long code = (unsigned long long)codeSelector;
    __asm__ volatile (
        "lgdt (%0)\n"
        "movw %w2, %%ds\n"
        "movw %w2, %%es\n"
        "movw %w2, %%ss\n"
        "pushq %1\n"
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        :
        : "r"(pointer), "r"(code), "r"(dataSelector)
        : "rax", "memory");
}

const OrynKernelGdtState* OrynKernelGdtGetState(void)
{
    return &gGdtState;
}

unsigned short OrynKernelGdtCodeSelector(void)
{
    return ORYN_GDT_KERNEL_CODE_SELECTOR;
}

unsigned char OrynKernelGdtExceptionIstIndex(void)
{
    return ORYN_GDT_EXCEPTION_IST;
}

int OrynKernelGdtInit(void)
{
    OrynGdtPointer pointer;
    OrynGdtPointer loaded;
    unsigned long long istTop;

    KernelIoWriteString("[KERNEL] GDT: installing\n");
    ClearBytes(gGdt, sizeof(gGdt));
    ClearBytes(&gTss, sizeof(gTss));
    ClearBytes(&gGdtState, sizeof(gGdtState));

    istTop = (unsigned long long)&gExceptionIstStack[ORYN_GDT_IST_STACK_SIZE];
    gTss.Rsp0 = istTop;
    gTss.Ist1 = istTop;
    gTss.IoMapBase = sizeof(gTss);

    gGdt[1] = MakeSegmentDescriptor(0U, ORYN_GDT_SEGMENT_LIMIT,
        ORYN_GDT_ACCESS_CODE, ORYN_GDT_FLAGS_CODE);
    gGdt[2] = MakeSegmentDescriptor(0U, ORYN_GDT_SEGMENT_LIMIT,
        ORYN_GDT_ACCESS_DATA, ORYN_GDT_FLAGS_DATA);
    gGdt[3] = MakeSegmentDescriptor(0U, ORYN_GDT_SEGMENT_LIMIT,
        ORYN_GDT_ACCESS_USER_DATA, ORYN_GDT_FLAGS_DATA);
    gGdt[4] = MakeSegmentDescriptor(0U, ORYN_GDT_SEGMENT_LIMIT,
        ORYN_GDT_ACCESS_USER_CODE, ORYN_GDT_FLAGS_CODE);
    SetTssDescriptor(5U, (unsigned long long)&gTss, (unsigned int)(sizeof(gTss) - 1U));

    pointer.Limit = (unsigned short)(sizeof(gGdt) - 1U);
    pointer.Base = (unsigned long long)gGdt;
    LoadGdtAndSegments(&pointer,
        ORYN_GDT_KERNEL_CODE_SELECTOR,
        ORYN_GDT_KERNEL_DATA_SELECTOR);
    LoadTaskRegister(ORYN_GDT_TSS_SELECTOR);
    StoreGdt(&loaded);

    gGdtState.Installed = 1U;
    gGdtState.EntryCount = ORYN_GDT_ENTRY_COUNT;
    gGdtState.CodeSelector = ReadCs();
    gGdtState.DataSelector = ReadDs();
    gGdtState.TssSelector = ORYN_GDT_TSS_SELECTOR;
    gGdtState.TaskRegister = ReadTr();
    gGdtState.Limit = pointer.Limit;
    gGdtState.Base = pointer.Base;
    gGdtState.LoadedBase = loaded.Base;
    gGdtState.LoadedLimit = loaded.Limit;
    gGdtState.TssBase = (unsigned long long)&gTss;
    gGdtState.TssLimit = (unsigned int)(sizeof(gTss) - 1U);
    gGdtState.ExceptionIstTop = istTop;
    gGdtState.TssLoaded = (gGdtState.TaskRegister == ORYN_GDT_TSS_SELECTOR) ? 1U : 0U;
    gGdtState.Verified =
        (loaded.Base == pointer.Base &&
         loaded.Limit == pointer.Limit &&
         gGdtState.CodeSelector == ORYN_GDT_KERNEL_CODE_SELECTOR &&
         gGdtState.TssLoaded != 0U) ? 1U : 0U;
    return gGdtState.Verified ? 1 : 0;
}

void OrynKernelGdtPrintProof(void)
{
    if (gGdtState.Verified)
    {
        KernelIoWriteString("[KERNEL] PASS: GDT installed.\n");
    }
    else
    {
        KernelIoWriteString("[KERNEL] FAIL: GDT install verification failed.\n");
    }

    KernelIoWriteString("[KERNEL] GDT entries: ");
    KernelIoWriteDec64(gGdtState.EntryCount);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] GDT base: ");
    KernelIoWriteHex64(gGdtState.Base);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] GDT limit: ");
    KernelIoWriteHex64(gGdtState.Limit);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] GDT code selector: ");
    KernelIoWriteHex64(gGdtState.CodeSelector);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] GDT data selector: ");
    KernelIoWriteHex64(gGdtState.DataSelector);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] TSS selector: ");
    KernelIoWriteHex64(gGdtState.TaskRegister);
    KernelIoWriteString("\n");
    KernelIoWriteString(gGdtState.TssLoaded ?
        "[KERNEL] PASS: TSS loaded.\n" :
        "[KERNEL] FAIL: TSS not loaded.\n");
    KernelIoWriteString("[KERNEL] Exception IST stack top: ");
    KernelIoWriteHex64(gGdtState.ExceptionIstTop);
    KernelIoWriteString("\n");
}
