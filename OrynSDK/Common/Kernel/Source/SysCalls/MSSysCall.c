#include "MSSysCall.h"

static void CopyArguments(
    OrynSysCallPacket* packet,
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    packet->Arguments[0] = arg0;
    packet->Arguments[1] = arg1;
    packet->Arguments[2] = arg2;
    packet->Arguments[3] = arg3;
    packet->Arguments[4] = arg4;
    packet->Arguments[5] = arg5;
}

int MSSysCallTranslate(
    uint64_t number,
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    OrynSysCallPacket* packet)
{
    if (packet == 0)
    {
        return 0;
    }

    if (number == ORYN_MS_SYSCALL_DEBUG_PRINT ||
        number == ORYN_MS_SYSCALL_NT_WRITE_FILE_COMPAT)
    {
        OrynSysCallPreparePacket(packet, ORYN_SYSCALL_KIND_EVENT,
            ORYN_SYSCALL_NS_DEBUG, ORYN_SYSCALL_OP_DEBUG_WRITE_STRING);
        CopyArguments(packet, arg0, arg1, arg2, arg3, arg4, arg5);
        packet->DebugText = (const char*)arg1;
    }
    else if (number == ORYN_MS_SYSCALL_GET_VERSION)
    {
        OrynSysCallPreparePacket(packet, ORYN_SYSCALL_KIND_GET,
            ORYN_SYSCALL_NS_KERNEL, ORYN_SYSCALL_OP_KERNEL_VERSION);
        CopyArguments(packet, arg0, arg1, arg2, arg3, arg4, arg5);
    }
    else if (number == ORYN_MS_SYSCALL_PROCESS_EXIT)
    {
        OrynSysCallPreparePacket(packet, ORYN_SYSCALL_KIND_EVENT,
            ORYN_SYSCALL_NS_PROCESS, ORYN_SYSCALL_OP_PROCESS_EXIT);
        CopyArguments(packet, arg0, arg1, arg2, arg3, arg4, arg5);
    }
    else
    {
        OrynSysCallPreparePacket(packet, ORYN_SYSCALL_KIND_EVENT,
            ORYN_SYSCALL_NS_COMPAT, ORYN_SYSCALL_OP_COMPAT_UNKNOWN_PLATFORM_SYSCALL);
        CopyArguments(packet, arg0, arg1, arg2, arg3, arg4, arg5);
    }

    packet->Platform = ORYN_SYSCALL_PLATFORM_MS;
    packet->PlatformNumber = number;
    return 1;
}

int64_t MSSysCallDispatch(
    uint64_t number,
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    OrynSysCallPacket packet;
    if (!MSSysCallTranslate(number, arg0, arg1, arg2, arg3, arg4, arg5, &packet))
    {
        return ORYN_SYSCALL_STATUS_BAD_PACKET;
    }

    return OrynSysCallDispatch(&packet);
}
