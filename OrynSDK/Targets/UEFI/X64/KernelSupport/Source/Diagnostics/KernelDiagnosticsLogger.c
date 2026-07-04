#include "KernelDiagnosticsLogger.h"
#include "KernelIo.h"
#include "KernelScreenReport.h"

static unsigned long long gDiagnosticsLoggerWriteCount;

static int IsProofDetailText(const char* text)
{
    return text != 0 &&
        text[0] == '[' &&
        text[1] == 'K' &&
        text[2] == 'E' &&
        text[3] == 'R' &&
        text[4] == 'N' &&
        text[5] == 'E' &&
        text[6] == 'L' &&
        text[7] == ']';
}

void OrynKernelDiagnosticsLogText(const char* text)
{
    if (IsProofDetailText(text))
    {
        gDiagnosticsLoggerWriteCount += 1ULL;
    }

    KernelIoWriteString(text);
}

void OrynKernelDiagnosticsLogLine(const char* text)
{
    OrynKernelDiagnosticsLogText(text);
    KernelIoWriteString("\n");
}

void OrynKernelDiagnosticsLogHex64(unsigned long long value)
{
    KernelIoWriteHex64(value);
}

void OrynKernelDiagnosticsLogDec64(unsigned long long value)
{
    KernelIoWriteDec64(value);
}

void OrynKernelDiagnosticsLogHaltMessage(void)
{
    OrynKernelDiagnosticsLogLine("[KERNEL] System halted by Kernel-5.");
}

void OrynKernelDiagnosticsLoggerPrintOwnershipProof(void)
{
    OrynKernelScreenReportOk(0,
        "Serial proof detail is owned by Diagnostics/Logger only.");
    OrynKernelDiagnosticsLogText("[KERNEL] Diagnostics/Logger proof-detail writes: ");
    OrynKernelDiagnosticsLogDec64(gDiagnosticsLoggerWriteCount);
    OrynKernelDiagnosticsLogText("\n");
}
