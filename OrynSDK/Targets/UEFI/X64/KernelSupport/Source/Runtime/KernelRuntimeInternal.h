#ifndef ORYN_KERNEL_RUNTIME_INTERNAL_H
#define ORYN_KERNEL_RUNTIME_INTERNAL_H

#include "OrynBootInfo.h"

const OrynBootInfo* OrynKernelRuntimeEnter(const OrynBootInfo* bootInfo);
void OrynKernelRuntimeHaltForever(void);
void OrynKernelRuntimeExitForNonInteractiveVm(void);

#endif
