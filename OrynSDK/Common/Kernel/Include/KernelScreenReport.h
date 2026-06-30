#ifndef ORYN_KERNEL_SCREEN_REPORT_H
#define ORYN_KERNEL_SCREEN_REPORT_H

void OrynKernelScreenReportInit(void);
void OrynKernelScreenReportObserve(const char* line);
int OrynKernelScreenReportNormalizeStatusLine(
    const char* input,
    char* output,
    unsigned int outputSize);
int OrynKernelScreenReportLineStatus(const char* line);
void OrynKernelScreenReportOk(const char* category, const char* message);
void OrynKernelScreenReportWarn(const char* category, const char* message);
void OrynKernelScreenReportFail(const char* category, const char* message);
void OrynKernelScreenReportOkOrFail(
    int condition,
    const char* okMessage,
    const char* failMessage);
void OrynKernelScreenReportOkOrWarn(
    int condition,
    const char* okMessage,
    const char* warnMessage);
void OrynKernelScreenReportBeginFail(void);
void OrynKernelScreenReportBeginWarn(void);
void OrynKernelScreenReportPrint(void);
void OrynKernelScreenReportWriteStatusLine(
    const char* status,
    const char* category,
    unsigned int colour);

#endif
