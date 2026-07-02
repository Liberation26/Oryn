#include "SysCallPolicy.h"
#include "KernelIo.h"
#include "KernelScreenReport.h"

void OrynSysCallPolicyInit(OrynSysCallState* state)
{
    if (state == 0)
    {
        return;
    }

    state->AbiMinVersion = ORYN_SYSCALL_ABI_MIN_VERSION;
    state->AbiMaxVersion = ORYN_SYSCALL_ABI_MAX_VERSION;
    state->CredentialsPlaceholderReady = 1U;
    state->UserPointerLow = ORYN_SYSCALL_USER_LOW;
    state->UserPointerHigh = ORYN_SYSCALL_USER_HIGH;
    state->SyscallTraceEnabled = ORYN_SYSCALL_TRACE_DEBUG;
}

int OrynSysCallValidatePacket(const OrynSysCallPacket* packet, OrynSysCallState* state)
{
    if (state != 0)
    {
        state->PacketValidationChecks += 1ULL;
    }

    if (packet == 0 || packet->Magic != ORYN_SYSCALL_PACKET_MAGIC)
    {
        if (state != 0) { state->PacketValidationFailures += 1ULL; }
        return 0;
    }

    if (packet->Version < ORYN_SYSCALL_ABI_MIN_VERSION ||
        packet->Version > ORYN_SYSCALL_ABI_MAX_VERSION)
    {
        if (state != 0) { state->AbiVersionFailures += 1ULL; }
        return 0;
    }

    if (packet->Size != sizeof(OrynSysCallPacket))
    {
        if (state != 0) { state->InvalidPacketSizeFailures += 1ULL; }
        return 0;
    }

    return 1;
}

int OrynSysCallValidateUserPointer(uint64_t address, uint64_t size, OrynSysCallState* state)
{
    uint64_t last;

    if (state != 0)
    {
        state->UserPointerChecks += 1ULL;
    }

    if (address == 0ULL || size == 0ULL || address < ORYN_SYSCALL_USER_LOW)
    {
        if (state != 0) { state->UserPointerFailures += 1ULL; }
        return 0;
    }

    last = address + size - 1ULL;
    if (last < address || last > ORYN_SYSCALL_USER_HIGH)
    {
        if (state != 0) { state->UserPointerFailures += 1ULL; }
        return 0;
    }

    return 1;
}

int OrynSysCallValidateArguments(const OrynSysCallPacket* packet, OrynSysCallState* state)
{
    if (state != 0)
    {
        state->ArgumentValidationChecks += 1ULL;
    }

    if (packet == 0)
    {
        if (state != 0) { state->ArgumentValidationFailures += 1ULL; }
        return 0;
    }

    if (packet->Namespace == ORYN_SYSCALL_NS_DEBUG &&
        packet->Operation == ORYN_SYSCALL_OP_DEBUG_WRITE_STRING &&
        packet->DebugText == 0 && packet->Arguments[1] != 0ULL)
    {
        return OrynSysCallValidateUserPointer(packet->Arguments[1], packet->Arguments[0], state);
    }

    return 1;
}

int64_t OrynSysCallMapStatusToErrno(int64_t status)
{
    if (status == ORYN_SYSCALL_STATUS_OK) { return 0LL; }
    if (status == ORYN_SYSCALL_STATUS_BAD_PACKET) { return -ORYN_SYSCALL_ERRNO_EINVAL; }
    if (status == ORYN_SYSCALL_STATUS_UNSUPPORTED) { return -ORYN_SYSCALL_ERRNO_ENOSYS; }
    if (status == ORYN_SYSCALL_STATUS_BAD_POINTER) { return -ORYN_SYSCALL_ERRNO_EFAULT; }
    return -ORYN_SYSCALL_ERRNO_ENOSYS;
}

void OrynSysCallTracePacket(const OrynSysCallPacket* packet, OrynSysCallState* state)
{
    if (state == 0 || packet == 0 || state->SyscallTraceEnabled == 0U)
    {
        return;
    }

    state->TracePackets += 1ULL;
    KernelIoWriteString("[KERNEL] DEBUG: SysCall trace kind=");
    KernelIoWriteHex64(packet->Kind);
    KernelIoWriteString(" ns=");
    KernelIoWriteHex64(packet->Namespace);
    KernelIoWriteString(" op=");
    KernelIoWriteHex64(packet->Operation);
    KernelIoWriteString("\n");
}

void OrynSysCallInstallCredentialPlaceholder(OrynSysCallPacket* packet, OrynSysCallState* state)
{
    if (packet == 0 || state == 0)
    {
        return;
    }

    packet->CredentialUid = ORYN_SYSCALL_CRED_USER_UID;
    packet->CredentialGid = ORYN_SYSCALL_CRED_USER_GID;
    state->CredentialPackets += 1ULL;
}
