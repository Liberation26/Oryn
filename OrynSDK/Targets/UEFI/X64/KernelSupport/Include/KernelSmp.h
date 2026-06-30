#ifndef ORYN_KERNEL_SMP_H
#define ORYN_KERNEL_SMP_H

#include "OrynBootInfo.h"

#define ORYN_SMP_MAX_CPUS 16U
#define ORYN_SMP_STACK_SIZE 16384U
#define ORYN_SMP_TRAMPOLINE_BASE 0x8000ULL
#define ORYN_SMP_TRAMPOLINE_VECTOR 0x08U

typedef struct OrynKernelSmpCpu
{
    unsigned int ProcessorUid;
    unsigned int LocalApicId;
    unsigned int Enabled;
    unsigned int IsBootstrapProcessor;
    unsigned int StartupAttempted;
    unsigned int Started;
} OrynKernelSmpCpu;

typedef struct OrynKernelSmpState
{
    unsigned int Initialized;
    unsigned int RsdpPresent;
    unsigned int AcpiChecksumOk;
    unsigned int MadtFound;
    unsigned int MadtLocalApicAddressValid;
    unsigned int DiscoveryComplete;
    unsigned int AcpiReadBeforeVirtualMemory;
    unsigned int LocalApicEntryCount;
    unsigned int EnabledCpuCount;
    unsigned int BootstrapApicId;
    unsigned int BootstrapCpuIndex;
    unsigned int ApplicationProcessorCount;
    unsigned int StartupAttemptCount;
    unsigned int StartupSuccessCount;
    unsigned int TrampolinePrepared;
    unsigned int TrampolineSize;
    unsigned int Cr3Below4G;
    unsigned int IpiPathReady;
    unsigned int InitIpiCount;
    unsigned int StartupIpiCount;
    unsigned int StartedCounter;
    unsigned long long LocalApicPhysicalBase;
    unsigned long long TrampolineBase;
    unsigned long long CurrentCr3;
    OrynKernelSmpCpu Cpus[ORYN_SMP_MAX_CPUS];
} OrynKernelSmpState;

int OrynKernelSmpDiscover(const OrynBootInfo* bootInfo);
int OrynKernelSmpInit(const OrynBootInfo* bootInfo);
const OrynKernelSmpState* OrynKernelSmpGetState(void);
void OrynKernelSmpPrintDiscoveryProof(const OrynKernelSmpState* state);
void OrynKernelSmpPrintProof(void);
void OrynKernelSmpApEntry(unsigned int localApicId);

#endif
