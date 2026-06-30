#ifndef ORYN_KERNEL_PANIC_H
#define ORYN_KERNEL_PANIC_H

#include "KernelLifecycle.h"
#include "OrynBootInfo.h"

typedef struct OrynKernelPanicReport
{
    unsigned long long Magic;
    unsigned long long Version;
    unsigned long long Sequence;
    const char* Reason;
    const char* Detail;
    unsigned long long Code;
    unsigned long long Vector;
    unsigned long long ErrorCode;
    unsigned long long Rip;
    unsigned long long Cs;
    unsigned long long Rflags;
    unsigned long long Cr2;
    OrynKernelLifecycleState LifecycleState;
    const OrynBootInfo* BootInfo;
    int Active;
    int ScreenAttempted;
    int ScreenShown;
    int ReportWritten;
    int HaltedByKernel;
} OrynKernelPanicReport;

void OrynKernelPanicInit(const OrynBootInfo* bootInfo);
void OrynKernelPanicBegin(
    const char* reason,
    const char* detail,
    unsigned long long code);
void OrynKernelPanicSetException(
    const char* exceptionName,
    unsigned long long vector,
    unsigned long long errorCode,
    unsigned long long rip,
    unsigned long long cs,
    unsigned long long rflags,
    unsigned long long cr2);
int OrynKernelPanicIsActive(void);
const OrynKernelPanicReport* OrynKernelPanicGetReport(void);
void OrynKernelPanicRenderScreen(void);
void OrynKernelPanicWriteReport(void);
void OrynKernelPanicHalt(void) __attribute__((noreturn));
void OrynKernelPanicRaiseException(
    const char* exceptionName,
    unsigned long long vector,
    unsigned long long errorCode,
    unsigned long long rip,
    unsigned long long cs,
    unsigned long long rflags,
    unsigned long long cr2) __attribute__((noreturn));

#endif
