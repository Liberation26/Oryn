#include "KernelPanic.h"
#include "KernelConsole.h"
#include "KernelIo.h"

#define ORYN_KERNEL_PANIC_MAGIC 0x4F52594E50414E49ULL
#define ORYN_KERNEL_PANIC_VERSION 1ULL

#ifndef ORYN_VM_INTERACTIVE_DISPLAY
#define ORYN_VM_INTERACTIVE_DISPLAY 0
#endif

static OrynKernelPanicReport gPanicReport;
static unsigned long long gPanicSequence;

static const char* TextOrDefault(const char* text, const char* fallback)
{
    return text == 0 ? fallback : text;
}

static void ClearReport(void)
{
    gPanicReport.Magic = ORYN_KERNEL_PANIC_MAGIC;
    gPanicReport.Version = ORYN_KERNEL_PANIC_VERSION;
    gPanicReport.Sequence = 0ULL;
    gPanicReport.Reason = "none";
    gPanicReport.Detail = "none";
    gPanicReport.Code = 0ULL;
    gPanicReport.Vector = 0ULL;
    gPanicReport.ErrorCode = 0ULL;
    gPanicReport.Rip = 0ULL;
    gPanicReport.Cs = 0ULL;
    gPanicReport.Rflags = 0ULL;
    gPanicReport.Cr2 = 0ULL;
    gPanicReport.LifecycleState = OrynKernelLifecycleCold;
    gPanicReport.BootInfo = 0;
    gPanicReport.Active = 0;
    gPanicReport.ScreenAttempted = 0;
    gPanicReport.ScreenShown = 0;
    gPanicReport.ReportWritten = 0;
    gPanicReport.HaltedByKernel = 0;
}

static void WriteTextField(const char* name, const char* value)
{
    KernelIoWriteString("[KERNEL] Panic ");
    KernelIoWriteString(name);
    KernelIoWriteString(": ");
    KernelIoWriteString(TextOrDefault(value, "none"));
    KernelIoWriteString("\n");
}

static void WriteHexField(const char* name, unsigned long long value)
{
    KernelIoWriteString("[KERNEL] Panic ");
    KernelIoWriteString(name);
    KernelIoWriteString(": ");
    KernelIoWriteHex64(value);
    KernelIoWriteString("\n");
}

static void WriteDecField(const char* name, unsigned long long value)
{
    KernelIoWriteString("[KERNEL] Panic ");
    KernelIoWriteString(name);
    KernelIoWriteString(": ");
    KernelIoWriteDec64(value);
    KernelIoWriteString("\n");
}

static void PanicScreenLine(const char* name, const char* value)
{
    KernelIoWriteString(name);
    KernelIoWriteString(TextOrDefault(value, "none"));
    KernelIoWriteString("\n");
}

static void PanicScreenHexLine(const char* name, unsigned long long value)
{
    KernelIoWriteString(name);
    KernelIoWriteHex64(value);
    KernelIoWriteString("\n");
}

static void StartPanic(
    const char* reason,
    const char* detail,
    unsigned long long code)
{
    if (!gPanicReport.Active)
    {
        gPanicReport.Sequence = ++gPanicSequence;
    }

    gPanicReport.Active = 1;
    gPanicReport.Reason = TextOrDefault(reason, "unspecified kernel panic");
    gPanicReport.Detail = TextOrDefault(detail, "none");
    gPanicReport.Code = code;
    OrynKernelLifecycleMarkPanic(gPanicReport.Reason);
    gPanicReport.LifecycleState = OrynKernelLifecycleGetState();
}

void OrynKernelPanicInit(const OrynBootInfo* bootInfo)
{
    ClearReport();
    gPanicReport.BootInfo = bootInfo;
    KernelIoWriteString("[KERNEL] PASS: Kernel-owned panic report path initialized.\n");
}

void OrynKernelPanicBegin(
    const char* reason,
    const char* detail,
    unsigned long long code)
{
    StartPanic(reason, detail, code);
}

void OrynKernelPanicSetException(
    const char* exceptionName,
    unsigned long long vector,
    unsigned long long errorCode,
    unsigned long long rip,
    unsigned long long cs,
    unsigned long long rflags,
    unsigned long long cr2)
{
    StartPanic("CPU exception", exceptionName, vector);
    gPanicReport.Vector = vector;
    gPanicReport.ErrorCode = errorCode;
    gPanicReport.Rip = rip;
    gPanicReport.Cs = cs;
    gPanicReport.Rflags = rflags;
    gPanicReport.Cr2 = cr2;
}

int OrynKernelPanicIsActive(void)
{
    return gPanicReport.Active;
}

const OrynKernelPanicReport* OrynKernelPanicGetReport(void)
{
    return &gPanicReport;
}

void OrynKernelPanicRenderScreen(void)
{
    if (!gPanicReport.Active)
    {
        return;
    }

    gPanicReport.ScreenAttempted = 1;
    if (!KConsole.IsAvailable())
    {
        gPanicReport.ScreenShown = 0;
        return;
    }

    KConsole.ClearScreen();
    KConsole.SetForegroundColour(KCONSOLE_COLOUR_FAIL);
    KernelIoWriteString("\n*** ORYN KERNEL PANIC ***\n");
    KernelIoWriteString("Kernel-owned panic screen active.\n");
    PanicScreenLine("Reason: ", gPanicReport.Reason);
    PanicScreenLine("Detail: ", gPanicReport.Detail);
    PanicScreenLine("Lifecycle: ", OrynKernelLifecycleStateName(gPanicReport.LifecycleState));
    PanicScreenHexLine("Code: ", gPanicReport.Code);
    PanicScreenHexLine("RIP: ", gPanicReport.Rip);
    PanicScreenHexLine("CR2: ", gPanicReport.Cr2);
    KernelIoWriteString("The kernel will not return to the loader.\n");
    KernelIoWriteString("A serial/debugcon panic report follows.\n\n");
    KConsole.ResetForegroundColour();
    gPanicReport.ScreenShown = 1;
}

void OrynKernelPanicWriteReport(void)
{
    if (!gPanicReport.Active)
    {
        return;
    }

    gPanicReport.LifecycleState = OrynKernelLifecycleGetState();
    KernelIoWriteString("[KERNEL] PANIC REPORT BEGIN\n");
    KernelIoWriteString("[KERNEL] FAIL: Kernel-owned panic path active.\n");
    KernelIoWriteString("[KERNEL] PASS: Panic report is stored in kernel-owned memory.\n");
    WriteDecField("sequence", gPanicReport.Sequence);
    WriteTextField("reason", gPanicReport.Reason);
    WriteTextField("detail", gPanicReport.Detail);
    WriteTextField("lifecycle", OrynKernelLifecycleStateName(gPanicReport.LifecycleState));
    WriteHexField("code", gPanicReport.Code);
    WriteHexField("vector", gPanicReport.Vector);
    WriteHexField("error code", gPanicReport.ErrorCode);
    WriteHexField("rip", gPanicReport.Rip);
    WriteHexField("cs", gPanicReport.Cs);
    WriteHexField("rflags", gPanicReport.Rflags);
    WriteHexField("cr2", gPanicReport.Cr2);
    WriteHexField("bootinfo", (unsigned long long)gPanicReport.BootInfo);
    KernelIoWriteString(gPanicReport.ScreenShown ?
        "[KERNEL] PASS: Kernel panic screen rendered by kernel console.\n" :
        "[KERNEL] WARN: Kernel panic screen unavailable; serial/debug report remains active.\n");
    KernelIoWriteString("[KERNEL] PASS: Kernel panic report written to serial and debugcon.\n");
    KernelIoWriteString(gPanicReport.HaltedByKernel ?
        "[KERNEL] PASS: Kernel panic halt path is kernel-owned.\n" :
        "[KERNEL] INFO: Kernel panic halt path pending.\n");
    KernelIoWriteString("[KERNEL] PANIC REPORT END\n");
    gPanicReport.ReportWritten = 1;
}

void OrynKernelPanicHalt(void)
{
    gPanicReport.HaltedByKernel = 1;
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleHalting);
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleHalted);
    OrynKernelPanicRenderScreen();
    OrynKernelLifecyclePrintProof();
    OrynKernelPanicWriteReport();
#if !ORYN_VM_INTERACTIVE_DISPLAY
    KernelIoExitQemuFailure();
#else
    KernelIoWriteString("[KERNEL] PASS: Interactive panic screen held open by kernel halt loop.\n");
#endif
    for (;;)
    {
        __asm__ volatile ("cli");
        __asm__ volatile ("hlt");
    }
}

void OrynKernelPanicRaiseException(
    const char* exceptionName,
    unsigned long long vector,
    unsigned long long errorCode,
    unsigned long long rip,
    unsigned long long cs,
    unsigned long long rflags,
    unsigned long long cr2)
{
    OrynKernelPanicSetException(exceptionName, vector, errorCode, rip, cs, rflags, cr2);
    OrynKernelPanicHalt();
}
