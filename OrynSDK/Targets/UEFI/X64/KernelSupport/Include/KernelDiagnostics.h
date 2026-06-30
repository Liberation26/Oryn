#ifndef ORYN_KERNEL_DIAGNOSTICS_H
#define ORYN_KERNEL_DIAGNOSTICS_H

#include "OrynBootInfo.h"

void OrynKernelDiagnosticsRunBootProofs(const OrynBootInfo* kernelBootInfo);
void OrynKernelDiagnosticsRunHaltProofs(void);

#endif
