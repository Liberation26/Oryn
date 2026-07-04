#ifndef ORYN_KERNEL_MODULE_START_PLAN_H
#define ORYN_KERNEL_MODULE_START_PLAN_H

#include "KernelBootInfo.h"
#include "KernelModuleManifest.h"

typedef struct OrynKernelModuleStartDecision
{
    int ShouldStart;
    int IsFailure;
    int IsWarning;
    const char* Reason;
} OrynKernelModuleStartDecision;

OrynKernelModuleStartDecision OrynKernelModuleStartPlanDecide(
    const OrynBootInfo* bootInfo,
    OrynKernelModuleId id);

int OrynKernelModuleStartPlanShouldStart(const OrynBootInfo* bootInfo, OrynKernelModuleId id);

#endif
