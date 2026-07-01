#ifndef ORYN_KERNEL_BOOT_PROOF_H
#define ORYN_KERNEL_BOOT_PROOF_H

#include "OrynBootInfo.h"

void OrynKernelBootProofRunSequence(const OrynBootInfo* kernelBootInfo);
void OrynKernelBootProofRunCategoryChecks(const OrynBootInfo* kernelBootInfo);

#endif
