#ifndef ORYN_KERNEL_USER_MODE_H
#define ORYN_KERNEL_USER_MODE_H

#include "KernelProcess.h"

#define ORYN_USER_MODE_TEST_ENTRY 0x0000000000400000ULL
#define ORYN_USER_MODE_TEST_STACK 0x0000000000700000ULL

typedef enum OrynKernelSyscallEntryKind
{
    OrynKernelSyscallEntryInterruptFallback = 1,
    OrynKernelSyscallEntrySyscallSysret = 2
} OrynKernelSyscallEntryKind;

typedef struct OrynKernelUserIretFrame
{
    unsigned long long Rip;
    unsigned long long Cs;
    unsigned long long Rflags;
    unsigned long long Rsp;
    unsigned long long Ss;
} OrynKernelUserIretFrame;

typedef struct OrynKernelUserModeState
{
    unsigned int Initialized;
    unsigned int ControlledProcessReady;
    unsigned int UserThreadReady;
    unsigned int IretFrameReady;
    unsigned int TssStackReady;
    unsigned int SyscallEntrySelected;
    unsigned int UsesSyscallSysret;
    unsigned int UsesInterruptFallback;
    unsigned long long KernelStackTop;
    unsigned long long UserEntry;
    unsigned long long UserStackTop;
    unsigned short UserCodeSelector;
    unsigned short UserDataSelector;
    OrynKernelUserIretFrame IretFrame;
} OrynKernelUserModeState;

int OrynKernelUserModeInit(void);
int OrynKernelUserModePrepareControlledTest(OrynKernelThread* thread);
int OrynKernelUserModeRunProof(OrynKernelThread* thread);
const OrynKernelUserModeState* OrynKernelUserModeGetState(void);
void OrynKernelUserModePrintProof(void);

#endif
