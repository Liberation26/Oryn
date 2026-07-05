#ifndef ORYN_KERNEL_RUNTIME_STARTUP_H
#define ORYN_KERNEL_RUNTIME_STARTUP_H

#include "OrynBootInfo.h"

typedef struct OrynKernelRuntimeStartupResult
{
    unsigned int Considered;
    unsigned int Started;
    unsigned int Skipped;
    unsigned int Failed;
    unsigned int MissingLinkRoots;
} OrynKernelRuntimeStartupResult;

OrynKernelRuntimeStartupResult OrynKernelRuntimeStartSelectedModules(const OrynBootInfo* bootInfo);
int OrynKernelRuntimeStartupSucceeded(const OrynKernelRuntimeStartupResult* result);

#endif
