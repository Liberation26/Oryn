#ifndef ORYN_KERNEL_SCREEN_REPORT_H
#define ORYN_KERNEL_SCREEN_REPORT_H

void OrynKernelScreenReportInit(void);
void OrynKernelScreenReportObserve(const char* line);
void OrynKernelScreenReportPrint(void);
void OrynKernelScreenReportWriteStatusLine(
    const char* status,
    const char* category,
    unsigned int colour);

#endif
