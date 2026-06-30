#include "KernelSysCallInterrupts.h"
#include "KernelIdt.h"
#include "KernelInterrupts.h"
#include "KernelIo.h"
#include "LinuxSysCall.h"
#include "MSSysCall.h"

static OrynKernelSysCallInterruptState gSysCallInterruptState;

static void ClearBytes(void* target, unsigned long long count)
{
    unsigned char* bytes = (unsigned char*)target;
    for (unsigned long long index = 0; index < count; ++index)
    {
        bytes[index] = 0U;
    }
}

static unsigned long long TextLength(const char* text)
{
    unsigned long long length = 0ULL;
    if (text == 0)
    {
        return 0ULL;
    }

    while (text[length] != 0)
    {
        length += 1ULL;
    }

    return length;
}

static void LinuxSysCallInterruptHandler(OrynIdtInterruptFrame* frame, void* context)
{
    int64_t status;
    (void)context;
    gSysCallInterruptState.LinuxInterrupts += 1ULL;
    status = LinuxSysCallDispatch(frame->Rax, frame->Rdi, frame->Rsi,
        frame->Rdx, frame->R10, frame->R8, frame->R9);
    frame->Rax = (unsigned long long)status;
}

static void MSSysCallInterruptHandler(OrynIdtInterruptFrame* frame, void* context)
{
    int64_t status;
    (void)context;
    gSysCallInterruptState.MsInterrupts += 1ULL;
    status = MSSysCallDispatch(frame->Rax, frame->Rdi, frame->Rsi,
        frame->Rdx, frame->R10, frame->R8, frame->R9);
    frame->Rax = (unsigned long long)status;
}

static unsigned long long InvokeLinuxSysCall(
    unsigned long long number,
    unsigned long long arg0,
    unsigned long long arg1,
    unsigned long long arg2,
    unsigned long long arg3,
    unsigned long long arg4,
    unsigned long long arg5)
{
    register unsigned long long rax __asm__("rax") = number;
    register unsigned long long rdi __asm__("rdi") = arg0;
    register unsigned long long rsi __asm__("rsi") = arg1;
    register unsigned long long rdx __asm__("rdx") = arg2;
    register unsigned long long r10 __asm__("r10") = arg3;
    register unsigned long long r8 __asm__("r8") = arg4;
    register unsigned long long r9 __asm__("r9") = arg5;

    __asm__ volatile ("int $0x80"
        : "+a"(rax)
        : "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10), "r"(r8), "r"(r9)
        : "memory");
    return rax;
}

static unsigned long long InvokeMSSysCall(
    unsigned long long number,
    unsigned long long arg0,
    unsigned long long arg1,
    unsigned long long arg2,
    unsigned long long arg3,
    unsigned long long arg4,
    unsigned long long arg5)
{
    register unsigned long long rax __asm__("rax") = number;
    register unsigned long long rdi __asm__("rdi") = arg0;
    register unsigned long long rsi __asm__("rsi") = arg1;
    register unsigned long long rdx __asm__("rdx") = arg2;
    register unsigned long long r10 __asm__("r10") = arg3;
    register unsigned long long r8 __asm__("r8") = arg4;
    register unsigned long long r9 __asm__("r9") = arg5;

    __asm__ volatile ("int $0x81"
        : "+a"(rax)
        : "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10), "r"(r8), "r"(r9)
        : "memory");
    return rax;
}

const OrynKernelSysCallInterruptState* OrynKernelSysCallInterruptsGetState(void)
{
    return &gSysCallInterruptState;
}

int OrynKernelSysCallInterruptsInit(void)
{
    ClearBytes(&gSysCallInterruptState, sizeof(gSysCallInterruptState));
    gSysCallInterruptState.Initialized = 1U;
    gSysCallInterruptState.LinuxVectorRegistered = OrynKernelInterruptsRegisterHandler(
        ORYN_KERNEL_LINUX_SYSCALL_VECTOR,
        LinuxSysCallInterruptHandler,
        0,
        "linux-syscall-vector") ? 1U : 0U;
    gSysCallInterruptState.MsVectorRegistered = OrynKernelInterruptsRegisterHandler(
        ORYN_KERNEL_MS_SYSCALL_VECTOR,
        MSSysCallInterruptHandler,
        0,
        "ms-syscall-vector") ? 1U : 0U;
    return gSysCallInterruptState.LinuxVectorRegistered &&
        gSysCallInterruptState.MsVectorRegistered;
}

int OrynKernelSysCallInterruptsRunProof(void)
{
    const char* linuxText = "[KERNEL] LinuxSysCall: write translated to SysCallEvent.\n";
    const char* msText = "[KERNEL] MSSysCall: debug print translated to SysCallEvent.\n";
    unsigned long long beforeUnknown;
    unsigned long long afterUnknown;
    const OrynSysCallState* state;
    unsigned long long status;

    gSysCallInterruptState.ProofRan = 1U;
    gSysCallInterruptState.LinuxBefore = gSysCallInterruptState.LinuxInterrupts;
    status = InvokeLinuxSysCall(ORYN_LINUX_SYSCALL_WRITE, 1ULL,
        (unsigned long long)linuxText, TextLength(linuxText), 0ULL, 0ULL, 0ULL);
    gSysCallInterruptState.LinuxAfter = gSysCallInterruptState.LinuxInterrupts;
    gSysCallInterruptState.LinuxProofPassed =
        status == ORYN_SYSCALL_STATUS_OK &&
        gSysCallInterruptState.LinuxAfter > gSysCallInterruptState.LinuxBefore ? 1U : 0U;

    state = OrynSysCallGetState();
    beforeUnknown = state->UnknownLinuxPackets;
    status = InvokeLinuxSysCall(0xFFFFFFFFULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL);
    afterUnknown = OrynSysCallGetState()->UnknownLinuxPackets;
    gSysCallInterruptState.UnknownLinuxProofPassed =
        status == (unsigned long long)ORYN_SYSCALL_STATUS_UNKNOWN && afterUnknown > beforeUnknown ? 1U : 0U;

    gSysCallInterruptState.MsBefore = gSysCallInterruptState.MsInterrupts;
    status = InvokeMSSysCall(ORYN_MS_SYSCALL_DEBUG_PRINT, 1ULL,
        (unsigned long long)msText, TextLength(msText), 0ULL, 0ULL, 0ULL);
    gSysCallInterruptState.MsAfter = gSysCallInterruptState.MsInterrupts;
    gSysCallInterruptState.MsProofPassed =
        status == ORYN_SYSCALL_STATUS_OK &&
        gSysCallInterruptState.MsAfter > gSysCallInterruptState.MsBefore ? 1U : 0U;

    beforeUnknown = OrynSysCallGetState()->UnknownMsPackets;
    status = InvokeMSSysCall(0xDEADBEEFULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL);
    afterUnknown = OrynSysCallGetState()->UnknownMsPackets;
    gSysCallInterruptState.UnknownMsProofPassed =
        status == (unsigned long long)ORYN_SYSCALL_STATUS_UNKNOWN && afterUnknown > beforeUnknown ? 1U : 0U;

    return gSysCallInterruptState.LinuxProofPassed &&
        gSysCallInterruptState.MsProofPassed &&
        gSysCallInterruptState.UnknownLinuxProofPassed &&
        gSysCallInterruptState.UnknownMsProofPassed;
}

void OrynKernelSysCallInterruptsPrintProof(void)
{
    KernelIoWriteString(gSysCallInterruptState.Initialized ?
        "[KERNEL] PASS: SysCall interrupt receiver initialized.\n" :
        "[KERNEL] FAIL: SysCall interrupt receiver not initialized.\n");
    KernelIoWriteString(gSysCallInterruptState.LinuxVectorRegistered ?
        "[KERNEL] PASS: LinuxSysCall translator registered.\n" :
        "[KERNEL] FAIL: LinuxSysCall translator was not registered.\n");
    KernelIoWriteString(gSysCallInterruptState.MsVectorRegistered ?
        "[KERNEL] PASS: MSSysCall translator registered.\n" :
        "[KERNEL] FAIL: MSSysCall translator was not registered.\n");
}

void OrynKernelSysCallInterruptsPrintRuntimeProof(void)
{
    KernelIoWriteString(gSysCallInterruptState.LinuxProofPassed ?
        "[KERNEL] PASS: Linux syscall vector 0x80 received and translated.\n" :
        "[KERNEL] FAIL: Linux syscall vector 0x80 was not translated.\n");
    KernelIoWriteString(gSysCallInterruptState.MsProofPassed ?
        "[KERNEL] PASS: MS syscall vector 0x81 received and translated.\n" :
        "[KERNEL] FAIL: MS syscall vector 0x81 was not translated.\n");
    KernelIoWriteString(gSysCallInterruptState.UnknownLinuxProofPassed ?
        "[KERNEL] PASS: Unknown Linux syscall was reported as debug.\n" :
        "[KERNEL] FAIL: Unknown Linux syscall was not reported.\n");
    KernelIoWriteString(gSysCallInterruptState.UnknownMsProofPassed ?
        "[KERNEL] PASS: Unknown MS syscall was reported as debug.\n" :
        "[KERNEL] FAIL: Unknown MS syscall was not reported.\n");
    KernelIoWriteString(
        gSysCallInterruptState.LinuxProofPassed && gSysCallInterruptState.MsProofPassed ?
        "[KERNEL] PASS: Platform syscalls translate into Get/Set/Event packets.\n" :
        "[KERNEL] FAIL: Platform syscall translation proof incomplete.\n");
}
