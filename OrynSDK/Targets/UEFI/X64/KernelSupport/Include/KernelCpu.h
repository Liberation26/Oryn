#ifndef ORYN_KERNEL_CPU_H
#define ORYN_KERNEL_CPU_H

typedef struct OrynKernelCpuFeatures
{
    unsigned int Detected;
    unsigned int MaxBasicLeaf;
    unsigned int MaxExtendedLeaf;
    unsigned int HasLocalApic;
    unsigned int HasX2Apic;
    unsigned int HasTscDeadline;
    unsigned int HasInvariantTsc;
    char Vendor[13];
} OrynKernelCpuFeatures;

void OrynKernelCpuDetect(void);
const OrynKernelCpuFeatures* OrynKernelCpuGetFeatures(void);
void OrynKernelCpuPrintFeatures(void);

#endif
