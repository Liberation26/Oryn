#ifndef ORYN_KERNEL_IDT_H
#define ORYN_KERNEL_IDT_H

#define ORYN_IDT_ENTRY_COUNT 256U
#define ORYN_IDT_EXCEPTION_COUNT 32U
#define ORYN_IDT_GATE_INTERRUPT 0x8EU

typedef struct OrynKernelIdtState
{
    unsigned int Installed;
    unsigned int Verified;
    unsigned int EntryCount;
    unsigned short CodeSelector;
    unsigned char ExceptionIst;
    unsigned short Limit;
    unsigned long long Base;
    unsigned long long LoadedBase;
    unsigned short LoadedLimit;
} OrynKernelIdtState;

typedef struct OrynIdtInterruptFrame
{
    unsigned long long Rax;
    unsigned long long Rbx;
    unsigned long long Rcx;
    unsigned long long Rdx;
    unsigned long long Rbp;
    unsigned long long Rsi;
    unsigned long long Rdi;
    unsigned long long R8;
    unsigned long long R9;
    unsigned long long R10;
    unsigned long long R11;
    unsigned long long R12;
    unsigned long long R13;
    unsigned long long R14;
    unsigned long long R15;
    unsigned long long Vector;
    unsigned long long ErrorCode;
    unsigned long long Rip;
    unsigned long long Cs;
    unsigned long long Rflags;
} OrynIdtInterruptFrame;

int OrynKernelIdtInit(void);
const OrynKernelIdtState* OrynKernelIdtGetState(void);
void OrynKernelIdtPrintProof(void);
void OrynIdtDispatch(OrynIdtInterruptFrame* frame);

#endif
