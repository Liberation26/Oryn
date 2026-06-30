#ifndef ORYN_KERNEL_SYSCALL_INTERRUPTS_H
#define ORYN_KERNEL_SYSCALL_INTERRUPTS_H

#define ORYN_KERNEL_LINUX_SYSCALL_VECTOR 0x80U
#define ORYN_KERNEL_MS_SYSCALL_VECTOR 0x81U

typedef struct OrynKernelSysCallInterruptState
{
    unsigned int Initialized;
    unsigned int LinuxVectorRegistered;
    unsigned int MsVectorRegistered;
    unsigned long long LinuxInterrupts;
    unsigned long long MsInterrupts;
    unsigned long long LinuxBefore;
    unsigned long long LinuxAfter;
    unsigned long long MsBefore;
    unsigned long long MsAfter;
    unsigned int ProofRan;
    unsigned int LinuxProofPassed;
    unsigned int MsProofPassed;
    unsigned int UnknownLinuxProofPassed;
    unsigned int UnknownMsProofPassed;
} OrynKernelSysCallInterruptState;

int OrynKernelSysCallInterruptsInit(void);
const OrynKernelSysCallInterruptState* OrynKernelSysCallInterruptsGetState(void);
int OrynKernelSysCallInterruptsRunProof(void);
void OrynKernelSysCallInterruptsPrintProof(void);
void OrynKernelSysCallInterruptsPrintRuntimeProof(void);

#endif
