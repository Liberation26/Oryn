#ifndef ORYN_KERNEL_SCREEN_REPORT_H
#define ORYN_KERNEL_SCREEN_REPORT_H

void OrynKernelScreenReportInit(void);
void OrynKernelScreenReportObserve(const char* line);
int OrynKernelScreenReportNormalizeStatusLine(
    const char* input,
    char* output,
    unsigned int outputSize);
void OrynKernelScreenReportOk(const char* category, const char* message);
void OrynKernelScreenReportWarn(const char* category, const char* message);
void OrynKernelScreenReportFail(const char* category, const char* message);
void OrynKernelScreenReportPrint(void);
void OrynKernelScreenReportWriteStatusLine(
    const char* status,
    const char* category,
    unsigned int colour);

#endif
