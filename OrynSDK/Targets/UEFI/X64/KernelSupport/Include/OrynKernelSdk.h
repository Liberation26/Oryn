#ifndef ORYN_KERNEL_SDK_H
#define ORYN_KERNEL_SDK_H

#include "OrynBootInfo.h"

typedef struct OrynKernelSdkContext
{
    const OrynBootInfo* BootInfo;
    const char* KernelName;
    unsigned int SelectedModuleCount;
    unsigned int MissingModuleLinkRootCount;
    unsigned int RuntimeEntered;
} OrynKernelSdkContext;

typedef void (*OrynKernelSdkMain)(OrynKernelSdkContext* kernel);

typedef struct OrynKernelSdkApplication
{
    const char* KernelName;
    OrynKernelSdkMain Main;
} OrynKernelSdkApplication;

void OrynKernelSdkStart(
    const OrynBootInfo* bootInfo,
    const OrynKernelSdkApplication* application);

void OrynKernelSdkWrite(OrynKernelSdkContext* kernel, const char* text);
void OrynKernelSdkWriteLine(OrynKernelSdkContext* kernel, const char* text);
void OrynKernelSdkRunBootProof(OrynKernelSdkContext* kernel);
void OrynKernelSdkReportOk(OrynKernelSdkContext* kernel, const char* message);
void OrynKernelSdkReportFail(OrynKernelSdkContext* kernel, const char* message);
void OrynKernelSdkHalt(OrynKernelSdkContext* kernel);

#define ORYN_KERNEL_APPLICATION(kernelName, mainFunction) \
    static const OrynKernelSdkApplication gOrynKernelSdkApplication = \
    { \
        kernelName, \
        mainFunction \
    }; \
    void KernelStart(const OrynBootInfo* bootInfo) \
    { \
        OrynKernelSdkStart(bootInfo, &gOrynKernelSdkApplication); \
    }

#endif
