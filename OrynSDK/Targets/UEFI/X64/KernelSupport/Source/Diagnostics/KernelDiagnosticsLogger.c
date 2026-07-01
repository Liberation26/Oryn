#include "KernelDiagnosticsLogger.h"
#include "KernelIo.h"

void OrynKernelDiagnosticsLogText(const char* text)
{
    KernelIoWriteString(text);
}

void OrynKernelDiagnosticsLogLine(const char* text)
{
    KernelIoWriteString(text);
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
