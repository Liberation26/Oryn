#include "KernelInterrupts.h"
#include "KernelApic.h"
#include "KernelIo.h"
#include "KernelPic.h"
#include "KernelPortIo.h"

#define ORYN_QEMU_EXIT_PORT 0xF4U
#define ORYN_PIT_CHANNEL0 0x40U
#define ORYN_PIT_COMMAND 0x43U
#define ORYN_PIT_MODE0_LOHI 0x30U
#define ORYN_PIT_PROOF_DIVISOR 1193U

typedef struct OrynInterruptHandlerSlot
{
    OrynKernelInterruptHandler Handler;
    void* Context;
    const char* Name;
} OrynInterruptHandlerSlot;

static OrynKernelInterruptState gInterruptState;
static OrynInterruptHandlerSlot gHandlers[ORYN_INTERRUPT_VECTOR_COUNT];
static unsigned long long gVectorCounters[ORYN_INTERRUPT_VECTOR_COUNT];

static void ClearBytes(void* target, unsigned long long count)
{
    unsigned char* bytes = (unsigned char*)target;
    for (unsigned long long index = 0; index < count; ++index)
    {
        bytes[index] = 0U;
    }
}

static void Out32(unsigned short port, unsigned int value)
{
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static unsigned long long ReadRflags(void)
{
    unsigned long long value;
    __asm__ volatile ("pushfq; popq %0" : "=r"(value) :: "memory");
    return value;
}

static unsigned long long ReadCr2(void)
{
    unsigned long long value;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(value));
    return value;
}

static const char* ExceptionName(unsigned long long vector)
{
    static const char* names[ORYN_INTERRUPT_EXCEPTION_COUNT] =
    {
        "Divide Error", "Debug", "NMI", "Breakpoint",
        "Overflow", "Bound Range", "Invalid Opcode", "Device Not Available",
        "Double Fault", "Coprocessor Segment", "Invalid TSS", "Segment Not Present",
        "Stack Segment", "General Protection", "Page Fault", "Reserved",
        "x87 Floating Point", "Alignment Check", "Machine Check", "SIMD Floating Point",
        "Virtualization", "Control Protection", "Reserved", "Reserved",
        "Reserved", "Reserved", "Reserved", "Reserved",
        "Hypervisor Injection", "VMM Communication", "Security", "Reserved"
    };

    if (vector < ORYN_INTERRUPT_EXCEPTION_COUNT)
    {
        return names[vector];
    }

    return "Interrupt";
}

static void HaltAfterException(void)
{
    KernelIoWriteString("[KERNEL] FAIL: CPU exception trapped by interrupt dispatcher.\n");
    Out32(ORYN_QEMU_EXIT_PORT, 0x11U);
    for (;;)
    {
        __asm__ volatile ("cli");
        __asm__ volatile ("hlt");
    }
}

static void PrintExceptionReport(OrynIdtInterruptFrame* frame)
{
    KernelIoWriteString("[KERNEL] EXCEPTION: ");
    KernelIoWriteString(ExceptionName(frame->Vector));
    KernelIoWriteString(" vector ");
    KernelIoWriteDec64(frame->Vector);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] Exception error code: ");
    KernelIoWriteHex64(frame->ErrorCode);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] Exception RIP: ");
    KernelIoWriteHex64(frame->Rip);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] Exception CS: ");
    KernelIoWriteHex64(frame->Cs);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] Exception RFLAGS: ");
    KernelIoWriteHex64(frame->Rflags);
    KernelIoWriteString("\n");
    if (frame->Vector == 14ULL)
    {
        gInterruptState.LastCr2 = ReadCr2();
        KernelIoWriteString("[KERNEL] Page fault CR2: ");
        KernelIoWriteHex64(gInterruptState.LastCr2);
        KernelIoWriteString("\n");
    }
}

static void SendInterruptEoi(unsigned int vector)
{
    if (vector >= ORYN_INTERRUPT_IRQ_BASE && vector < ORYN_INTERRUPT_IRQ_LIMIT)
    {
        OrynKernelPicSendEoi(vector - ORYN_INTERRUPT_IRQ_BASE);
        gInterruptState.PicEoiCount += 1ULL;
        gInterruptState.EoiCount += 1ULL;
        return;
    }

    if (OrynKernelApicGetState()->Initialized)
    {
        OrynKernelApicSendEoi();
        gInterruptState.ApicEoiCount += 1ULL;
        gInterruptState.EoiCount += 1ULL;
    }
}

static void PicTimerInterruptHandler(OrynIdtInterruptFrame* frame, void* context)
{
    (void)frame;
    (void)context;
    gInterruptState.PicTimerInterrupts += 1ULL;
}

static void ApicTimerInterruptHandler(OrynIdtInterruptFrame* frame, void* context)
{
    (void)frame;
    (void)context;
    gInterruptState.ApicTimerInterrupts += 1ULL;
}

static void ProgramPitOneShot(unsigned int divisor)
{
    OrynPortOut8(ORYN_PIT_COMMAND, ORYN_PIT_MODE0_LOHI);
    OrynPortOut8(ORYN_PIT_CHANNEL0, (unsigned char)(divisor & 0xFFU));
    OrynPortOut8(ORYN_PIT_CHANNEL0, (unsigned char)((divisor >> 8) & 0xFFU));
}

const OrynKernelInterruptState* OrynKernelInterruptsGetState(void)
{
    return &gInterruptState;
}

int OrynKernelInterruptsInit(void)
{
    ClearBytes(&gInterruptState, sizeof(gInterruptState));
    ClearBytes(gHandlers, sizeof(gHandlers));
    ClearBytes(gVectorCounters, sizeof(gVectorCounters));
    gInterruptState.Initialized = 1U;
    gInterruptState.HandlerSlots = ORYN_INTERRUPT_VECTOR_COUNT;
    gInterruptState.InterruptsEnabled = OrynKernelInterruptsAreEnabled();
    return 1;
}

int OrynKernelInterruptsRegisterHandler(
    unsigned int vector,
    OrynKernelInterruptHandler handler,
    void* context,
    const char* name)
{
    if (vector >= ORYN_INTERRUPT_VECTOR_COUNT || handler == 0)
    {
        return 0;
    }

    if (gHandlers[vector].Handler == 0)
    {
        gInterruptState.RegisteredHandlers += 1U;
    }

    gHandlers[vector].Handler = handler;
    gHandlers[vector].Context = context;
    gHandlers[vector].Name = name;
    return 1;
}

unsigned long long OrynKernelInterruptsGetVectorCount(unsigned int vector)
{
    if (vector >= ORYN_INTERRUPT_VECTOR_COUNT)
    {
        return 0ULL;
    }

    return gVectorCounters[vector];
}

void OrynKernelInterruptsEnable(void)
{
    __asm__ volatile ("sti" ::: "memory");
    gInterruptState.InterruptsEnabled = OrynKernelInterruptsAreEnabled();
}

void OrynKernelInterruptsDisable(void)
{
    __asm__ volatile ("cli" ::: "memory");
    gInterruptState.InterruptsEnabled = OrynKernelInterruptsAreEnabled();
}

unsigned int OrynKernelInterruptsAreEnabled(void)
{
    return ((ReadRflags() & 0x200ULL) != 0ULL) ? 1U : 0U;
}

void OrynKernelInterruptsDispatch(OrynIdtInterruptFrame* frame)
{
    unsigned int vector;
    OrynInterruptHandlerSlot* slot;

    if (frame == 0)
    {
        return;
    }

    vector = (unsigned int)(frame->Vector & 0xFFULL);
    gInterruptState.TotalDispatches += 1ULL;
    gVectorCounters[vector] += 1ULL;
    gInterruptState.LastVector = vector;
    gInterruptState.LastErrorCodeLow = (unsigned int)(frame->ErrorCode & 0xFFFFFFFFULL);
    gInterruptState.LastRip = frame->Rip;

    if (vector < ORYN_INTERRUPT_EXCEPTION_COUNT)
    {
        gInterruptState.ExceptionDispatches += 1ULL;
        PrintExceptionReport(frame);
        HaltAfterException();
    }

    gInterruptState.HardwareDispatches += 1ULL;
    slot = &gHandlers[vector];
    if (slot->Handler != 0)
    {
        slot->Handler(frame, slot->Context);
    }

    SendInterruptEoi(vector);
}

int OrynKernelInterruptsRunPicTimerProof(void)
{
    unsigned long long before;
    unsigned long long after;
    unsigned int waited;

    gInterruptState.PicTimerProofRan = 1U;
    if (!gInterruptState.Initialized)
    {
        return 0;
    }

    if (!OrynKernelInterruptsRegisterHandler(
        ORYN_INTERRUPT_IRQ_BASE,
        PicTimerInterruptHandler,
        0,
        "pic-pit-irq0"))
    {
        return 0;
    }

    OrynKernelPicMaskAll();
    before = OrynKernelInterruptsGetVectorCount(ORYN_INTERRUPT_IRQ_BASE);
    gInterruptState.PicTimerBefore = before;
    ProgramPitOneShot(ORYN_PIT_PROOF_DIVISOR);
    OrynKernelPicSetIrqMask(0U, 0);
    OrynKernelInterruptsEnable();

    for (waited = 0U; waited < 50000000U; ++waited)
    {
        after = OrynKernelInterruptsGetVectorCount(ORYN_INTERRUPT_IRQ_BASE);
        if (after > before)
        {
            break;
        }

        __asm__ volatile ("pause");
    }

    OrynKernelInterruptsDisable();
    OrynKernelPicMaskAll();
    after = OrynKernelInterruptsGetVectorCount(ORYN_INTERRUPT_IRQ_BASE);
    gInterruptState.PicTimerAfter = after;
    gInterruptState.PicTimerProofPassed = (after > before) ? 1U : 0U;
    return gInterruptState.PicTimerProofPassed ? 1 : 0;
}

int OrynKernelInterruptsRunApicTimerProof(void)
{
    unsigned long long before;
    unsigned long long after;
    unsigned int waited;

    gInterruptState.ApicTimerProofRan = 1U;
    if (!gInterruptState.Initialized)
    {
        return 0;
    }

    if (!OrynKernelInterruptsRegisterHandler(
        ORYN_INTERRUPT_APIC_TIMER_VECTOR,
        ApicTimerInterruptHandler,
        0,
        "local-apic-timer"))
    {
        return 0;
    }

    before = OrynKernelInterruptsGetVectorCount(ORYN_INTERRUPT_APIC_TIMER_VECTOR);
    gInterruptState.ApicTimerBefore = before;
    if (!OrynKernelApicStartOneShotTimer(ORYN_INTERRUPT_APIC_TIMER_VECTOR, 1000000U, 0x3U))
    {
        OrynKernelApicMaskTimer();
        return 0;
    }

    OrynKernelInterruptsEnable();
    for (waited = 0U; waited < 50000000U; ++waited)
    {
        after = OrynKernelInterruptsGetVectorCount(ORYN_INTERRUPT_APIC_TIMER_VECTOR);
        if (after > before)
        {
            break;
        }

        __asm__ volatile ("pause");
    }

    OrynKernelInterruptsDisable();
    OrynKernelApicMaskTimer();
    after = OrynKernelInterruptsGetVectorCount(ORYN_INTERRUPT_APIC_TIMER_VECTOR);
    gInterruptState.ApicTimerAfter = after;
    gInterruptState.ApicTimerProofPassed = (after > before) ? 1U : 0U;
    return gInterruptState.ApicTimerProofPassed ? 1 : 0;
}

void OrynKernelInterruptsPrintProof(void)
{
    KernelIoWriteString(gInterruptState.Initialized ?
        "[KERNEL] PASS: Interrupt dispatcher initialized.\n" :
        "[KERNEL] FAIL: Interrupt dispatcher not initialized.\n");
    KernelIoWriteString(gInterruptState.HandlerSlots == ORYN_INTERRUPT_VECTOR_COUNT ?
        "[KERNEL] PASS: Interrupt handler table ready for 256 vectors.\n" :
        "[KERNEL] FAIL: Interrupt handler table size invalid.\n");
    KernelIoWriteString("[KERNEL] Interrupt registered handlers: ");
    KernelIoWriteDec64(gInterruptState.RegisteredHandlers);
    KernelIoWriteString("\n");
    KernelIoWriteString(OrynKernelInterruptsAreEnabled() ?
        "[KERNEL] WARN: CPU interrupts are currently enabled.\n" :
        "[KERNEL] PASS: CPU interrupts are currently disabled for controlled boot.\n");
}

void OrynKernelInterruptsPrintRuntimeProof(void)
{
    KernelIoWriteString(gInterruptState.PicTimerProofPassed ?
        "[KERNEL] PASS: PIC IRQ0 interrupt fired through IDT dispatch.\n" :
        "[KERNEL] FAIL: PIC IRQ0 interrupt did not fire through IDT dispatch.\n");
    KernelIoWriteString(gInterruptState.PicTimerAfter > gInterruptState.PicTimerBefore ?
        "[KERNEL] PASS: PIC IRQ0 handler counter updated.\n" :
        "[KERNEL] FAIL: PIC IRQ0 handler counter did not update.\n");
    KernelIoWriteString(gInterruptState.PicEoiCount != 0ULL ?
        "[KERNEL] PASS: PIC EOI path executed.\n" :
        "[KERNEL] FAIL: PIC EOI path did not execute.\n");
    KernelIoWriteString(gInterruptState.ApicTimerProofPassed ?
        "[KERNEL] PASS: APIC timer interrupt fired through IDT dispatch.\n" :
        "[KERNEL] FAIL: APIC timer interrupt did not fire through IDT dispatch.\n");
    KernelIoWriteString(gInterruptState.ApicTimerAfter > gInterruptState.ApicTimerBefore ?
        "[KERNEL] PASS: APIC timer IRQ counter updated by interrupt dispatch.\n" :
        "[KERNEL] FAIL: APIC timer IRQ counter did not update.\n");
    KernelIoWriteString(gInterruptState.ApicEoiCount != 0ULL ?
        "[KERNEL] PASS: APIC EOI path executed.\n" :
        "[KERNEL] FAIL: APIC EOI path did not execute.\n");
    KernelIoWriteString(
        gInterruptState.PicTimerProofPassed && gInterruptState.ApicTimerProofPassed ?
        "[KERNEL] PASS: Interrupts work from PIC upward through APIC/APIC2.\n" :
        "[KERNEL] FAIL: Interrupt chain from PIC upward is incomplete.\n");
    KernelIoWriteString("[KERNEL] Interrupt total dispatches: ");
    KernelIoWriteDec64(gInterruptState.TotalDispatches);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] Interrupt PIC IRQ0 vector count: ");
    KernelIoWriteDec64(gVectorCounters[ORYN_INTERRUPT_IRQ_BASE]);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] Interrupt APIC timer vector count: ");
    KernelIoWriteDec64(gVectorCounters[ORYN_INTERRUPT_APIC_TIMER_VECTOR]);
    KernelIoWriteString("\n");
}
