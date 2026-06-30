#include "KernelApic.h"
#include "KernelCpu.h"
#include "KernelIo.h"
#include "KernelMsr.h"

#define ORYN_MSR_APIC_BASE 0x1BU
#define ORYN_MSR_X2APIC_ID 0x802U
#define ORYN_MSR_X2APIC_VERSION 0x803U
#define ORYN_MSR_X2APIC_EOI 0x80BU
#define ORYN_MSR_X2APIC_SVR 0x80FU
#define ORYN_MSR_X2APIC_LVT_TIMER 0x832U
#define ORYN_MSR_X2APIC_TIMER_INITIAL 0x838U
#define ORYN_MSR_X2APIC_TIMER_CURRENT 0x839U
#define ORYN_MSR_X2APIC_TIMER_DIVIDE 0x83EU
#define ORYN_APIC_BASE_ENABLE 0x800ULL
#define ORYN_APIC_BASE_X2APIC 0x400ULL
#define ORYN_APIC_BASE_ADDRESS_MASK 0x000FFFFFF000ULL
#define ORYN_APIC_REG_ID 0x020U
#define ORYN_APIC_REG_VERSION 0x030U
#define ORYN_APIC_REG_EOI 0x0B0U
#define ORYN_APIC_REG_SVR 0x0F0U
#define ORYN_APIC_REG_LVT_TIMER 0x320U
#define ORYN_APIC_REG_TIMER_INITIAL 0x380U
#define ORYN_APIC_REG_TIMER_CURRENT 0x390U
#define ORYN_APIC_REG_TIMER_DIVIDE 0x3E0U
#define ORYN_APIC_SPURIOUS_VECTOR 0xFFU
#define ORYN_APIC_SOFTWARE_ENABLE 0x100U
#define ORYN_APIC_LVT_MASKED 0x10000U

static OrynKernelApicState gApicState;

static volatile unsigned int* ApicRegister(unsigned int offset)
{
    return (volatile unsigned int*)(gApicState.LocalApicPhysicalBase + offset);
}

static unsigned int ApicRead(unsigned int offset)
{
    return *ApicRegister(offset);
}

static void ApicWrite(unsigned int offset, unsigned int value)
{
    *ApicRegister(offset) = value;
    (void)ApicRead(ORYN_APIC_REG_ID);
}

static void BusyDelay(void)
{
    for (volatile unsigned int index = 0U; index < 200000U; ++index)
    {
    }
}

static void ClearState(void)
{
    unsigned char* bytes = (unsigned char*)&gApicState;
    for (unsigned int index = 0U; index < sizeof(gApicState); ++index)
    {
        bytes[index] = 0U;
    }
}

static void ProbeApic2Timer(void)
{
    OrynMsrWrite(ORYN_MSR_X2APIC_TIMER_DIVIDE, 0x3ULL);
    OrynMsrWrite(ORYN_MSR_X2APIC_LVT_TIMER, ORYN_APIC_LVT_MASKED | 0xEFU);
    OrynMsrWrite(ORYN_MSR_X2APIC_TIMER_INITIAL, 1000000ULL);
    gApicState.TimerCountBefore = OrynMsrRead(ORYN_MSR_X2APIC_TIMER_CURRENT);
    BusyDelay();
    gApicState.TimerCountAfter = OrynMsrRead(ORYN_MSR_X2APIC_TIMER_CURRENT);
    gApicState.TimerProbeRan = 1U;
    gApicState.TimerCountMoved =
        (gApicState.TimerCountAfter < gApicState.TimerCountBefore) ? 1U : 0U;
}

static void ProbeXApicTimer(void)
{
    ApicWrite(ORYN_APIC_REG_TIMER_DIVIDE, 0x3U);
    ApicWrite(ORYN_APIC_REG_LVT_TIMER, ORYN_APIC_LVT_MASKED | 0xEFU);
    ApicWrite(ORYN_APIC_REG_TIMER_INITIAL, 1000000U);
    gApicState.TimerCountBefore = ApicRead(ORYN_APIC_REG_TIMER_CURRENT);
    BusyDelay();
    gApicState.TimerCountAfter = ApicRead(ORYN_APIC_REG_TIMER_CURRENT);
    gApicState.TimerProbeRan = 1U;
    gApicState.TimerCountMoved =
        (gApicState.TimerCountAfter < gApicState.TimerCountBefore) ? 1U : 0U;
}

static void EnableApic2(void)
{
    OrynMsrWrite(
        ORYN_MSR_APIC_BASE,
        gApicState.BaseMsrBefore | ORYN_APIC_BASE_ENABLE | ORYN_APIC_BASE_X2APIC);
    gApicState.BaseMsrAfter = OrynMsrRead(ORYN_MSR_APIC_BASE);
    gApicState.Apic2Enabled =
        ((gApicState.BaseMsrAfter & ORYN_APIC_BASE_X2APIC) != 0ULL) ? 1U : 0U;
    OrynMsrWrite(
        ORYN_MSR_X2APIC_SVR,
        OrynMsrRead(ORYN_MSR_X2APIC_SVR) | ORYN_APIC_SOFTWARE_ENABLE | ORYN_APIC_SPURIOUS_VECTOR);
    gApicState.SoftwareEnabled =
        ((OrynMsrRead(ORYN_MSR_X2APIC_SVR) & ORYN_APIC_SOFTWARE_ENABLE) != 0ULL) ? 1U : 0U;
    gApicState.LocalApicId = (unsigned int)OrynMsrRead(ORYN_MSR_X2APIC_ID);
    unsigned long long version = OrynMsrRead(ORYN_MSR_X2APIC_VERSION);
    gApicState.Version = (unsigned int)(version & 0xFFULL);
    gApicState.MaxLvt = (unsigned int)((version >> 16) & 0xFFULL);
    ProbeApic2Timer();
}

static void EnableXApic(void)
{
    OrynMsrWrite(ORYN_MSR_APIC_BASE, gApicState.BaseMsrBefore | ORYN_APIC_BASE_ENABLE);
    gApicState.BaseMsrAfter = OrynMsrRead(ORYN_MSR_APIC_BASE);
    gApicState.XApicEnabled =
        ((gApicState.BaseMsrAfter & ORYN_APIC_BASE_ENABLE) != 0ULL) ? 1U : 0U;
    ApicWrite(ORYN_APIC_REG_SVR,
        ApicRead(ORYN_APIC_REG_SVR) | ORYN_APIC_SOFTWARE_ENABLE | ORYN_APIC_SPURIOUS_VECTOR);
    gApicState.SoftwareEnabled =
        ((ApicRead(ORYN_APIC_REG_SVR) & ORYN_APIC_SOFTWARE_ENABLE) != 0U) ? 1U : 0U;
    gApicState.LocalApicId = (ApicRead(ORYN_APIC_REG_ID) >> 24) & 0xFFU;
    unsigned int version = ApicRead(ORYN_APIC_REG_VERSION);
    gApicState.Version = version & 0xFFU;
    gApicState.MaxLvt = (version >> 16) & 0xFFU;
    ProbeXApicTimer();
}

int OrynKernelApicInit(int preferApic2)
{
    const OrynKernelCpuFeatures* cpu;
    ClearState();
    cpu = OrynKernelCpuGetFeatures();
    gApicState.CpuHasApic = cpu->HasLocalApic;
    gApicState.CpuHasApic2 = cpu->HasX2Apic;
    if (gApicState.CpuHasApic == 0U)
    {
        return 0;
    }

    gApicState.BaseMsrBefore = OrynMsrRead(ORYN_MSR_APIC_BASE);
    gApicState.LocalApicPhysicalBase =
        gApicState.BaseMsrBefore & ORYN_APIC_BASE_ADDRESS_MASK;
    if (gApicState.LocalApicPhysicalBase == 0ULL)
    {
        gApicState.LocalApicPhysicalBase = 0xFEE00000ULL;
    }

    if (preferApic2 && gApicState.CpuHasApic2)
    {
        EnableApic2();
    }
    else if ((gApicState.BaseMsrBefore & ORYN_APIC_BASE_X2APIC) != 0ULL)
    {
        EnableApic2();
    }
    else
    {
        EnableXApic();
    }

    gApicState.Initialized = gApicState.SoftwareEnabled;
    return gApicState.Initialized ? 1 : 0;
}

const OrynKernelApicState* OrynKernelApicGetState(void)
{
    return &gApicState;
}

void OrynKernelApicSendEoi(void)
{
    if (gApicState.Apic2Enabled)
    {
        OrynMsrWrite(ORYN_MSR_X2APIC_EOI, 0ULL);
    }
    else if (gApicState.XApicEnabled)
    {
        ApicWrite(ORYN_APIC_REG_EOI, 0U);
    }
}


int OrynKernelApicStartOneShotTimer(unsigned int vector, unsigned int initialCount, unsigned int divideMode)
{
    if (!gApicState.Initialized || vector < 32U || vector > 255U || initialCount == 0U)
    {
        return 0;
    }

    gApicState.TimerInterruptVector = vector;
    gApicState.TimerInterruptArmed = 1U;
    if (gApicState.Apic2Enabled)
    {
        OrynMsrWrite(ORYN_MSR_X2APIC_TIMER_DIVIDE, (unsigned long long)divideMode);
        OrynMsrWrite(ORYN_MSR_X2APIC_LVT_TIMER, (unsigned long long)(vector & 0xFFU));
        OrynMsrWrite(ORYN_MSR_X2APIC_TIMER_INITIAL, (unsigned long long)initialCount);
        return 1;
    }

    if (gApicState.XApicEnabled)
    {
        ApicWrite(ORYN_APIC_REG_TIMER_DIVIDE, divideMode);
        ApicWrite(ORYN_APIC_REG_LVT_TIMER, vector & 0xFFU);
        ApicWrite(ORYN_APIC_REG_TIMER_INITIAL, initialCount);
        return 1;
    }

    gApicState.TimerInterruptArmed = 0U;
    return 0;
}

void OrynKernelApicMaskTimer(void)
{
    if (gApicState.Apic2Enabled)
    {
        unsigned long long value = OrynMsrRead(ORYN_MSR_X2APIC_LVT_TIMER);
        OrynMsrWrite(ORYN_MSR_X2APIC_LVT_TIMER, value | ORYN_APIC_LVT_MASKED);
        OrynMsrWrite(ORYN_MSR_X2APIC_TIMER_INITIAL, 0ULL);
        gApicState.TimerInterruptArmed = 0U;
        return;
    }

    if (gApicState.XApicEnabled)
    {
        unsigned int value = ApicRead(ORYN_APIC_REG_LVT_TIMER);
        ApicWrite(ORYN_APIC_REG_LVT_TIMER, value | ORYN_APIC_LVT_MASKED);
        ApicWrite(ORYN_APIC_REG_TIMER_INITIAL, 0U);
        gApicState.TimerInterruptArmed = 0U;
    }
}

unsigned long long OrynKernelApicReadTimerCurrent(void)
{
    if (gApicState.Apic2Enabled)
    {
        return OrynMsrRead(ORYN_MSR_X2APIC_TIMER_CURRENT);
    }

    if (gApicState.XApicEnabled)
    {
        return (unsigned long long)ApicRead(ORYN_APIC_REG_TIMER_CURRENT);
    }

    return 0ULL;
}

void OrynKernelApicPrintProof(void)
{
    KernelIoWriteString(gApicState.CpuHasApic ?
        "[KERNEL] PASS: APIC CPU feature available.\n" :
        "[KERNEL] FAIL: APIC CPU feature unavailable.\n");
    KernelIoWriteString(gApicState.Apic2Enabled ?
        "[KERNEL] PASS: APIC2/x2APIC mode enabled.\n" :
        "[KERNEL] WARN: APIC2/x2APIC mode not enabled; using xAPIC if available.\n");
    KernelIoWriteString(gApicState.SoftwareEnabled ?
        "[KERNEL] PASS: Local APIC software enable bit set.\n" :
        "[KERNEL] FAIL: Local APIC software enable bit not set.\n");
    KernelIoWriteString("[KERNEL] Local APIC physical base: ");
    KernelIoWriteHex64(gApicState.LocalApicPhysicalBase);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] Local APIC ID/version/maxLVT: ");
    KernelIoWriteHex64(gApicState.LocalApicId);
    KernelIoWriteString(" / ");
    KernelIoWriteHex64(gApicState.Version);
    KernelIoWriteString(" / ");
    KernelIoWriteHex64(gApicState.MaxLvt);
    KernelIoWriteString("\n");
    KernelIoWriteString(gApicState.TimerCountMoved ?
        "[KERNEL] PASS: APIC timer counter moved in masked probe.\n" :
        "[KERNEL] WARN: APIC timer counter did not move in masked probe.\n");
}
