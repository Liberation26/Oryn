#ifndef ORYN_KERNEL_RUNTIME_H
#define ORYN_KERNEL_RUNTIME_H

#include "OrynBootInfo.h"

const OrynBootInfo* OrynKernelRuntimeEnter(const OrynBootInfo* bootInfo);
void OrynKernelRuntimeExitForNonInteractiveVm(void);
void OrynKernelRuntimeHaltForever(void);
void OrynKernelRuntimeEnterAndStartBootSequence(const OrynBootInfo* bootInfo);

#endif
