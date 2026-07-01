#ifndef ORYN_KERNEL_DIAGNOSTICS_LOGGER_H
#define ORYN_KERNEL_DIAGNOSTICS_LOGGER_H

void OrynKernelDiagnosticsLogText(const char* text);
void OrynKernelDiagnosticsLogLine(const char* text);
void OrynKernelDiagnosticsLogHex64(unsigned long long value);
void OrynKernelDiagnosticsLogDec64(unsigned long long value);
void OrynKernelDiagnosticsLogHaltMessage(void);

#endif
