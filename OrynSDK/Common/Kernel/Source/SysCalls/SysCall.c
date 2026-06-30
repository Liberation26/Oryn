#include "SysCall.h"
#include "LinuxSysCall.h"
#include "MSSysCall.h"
#include "KernelIo.h"
#include "OrynString.h"
#include "KernelScreenReport.h"

static OrynSysCallState gSysCallState;

static void ClearBytes(void* target, uint64_t count)
{
    unsigned char* bytes = (unsigned char*)target;
    for (uint64_t index = 0; index < count; ++index)
    {
        bytes[index] = 0U;
    }
}

static int IsValidPacket(const OrynSysCallPacket* packet)
{
    return packet != 0 &&
        packet->Magic == ORYN_SYSCALL_PACKET_MAGIC &&
        packet->Version == ORYN_SYSCALL_PACKET_VERSION &&
        packet->Size == sizeof(OrynSysCallPacket);
}

static void RecordPacket(const OrynSysCallPacket* packet)
{
    gSysCallState.TotalPackets += 1ULL;
    gSysCallState.LastPlatform = packet->Platform;
    gSysCallState.LastPlatformNumber = packet->PlatformNumber;
    gSysCallState.LastKind = packet->Kind;
    gSysCallState.LastNamespace = packet->Namespace;
    gSysCallState.LastOperation = packet->Operation;
    gSysCallState.LastStatus = packet->Status;

    if (packet->Platform == ORYN_SYSCALL_PLATFORM_LINUX)
    {
        gSysCallState.LinuxPackets += 1ULL;
    }
    else if (packet->Platform == ORYN_SYSCALL_PLATFORM_MS)
    {
        gSysCallState.MsPackets += 1ULL;
    }
}

static void WritePacketField(const char* label, uint64_t value)
{
    KernelIoWriteString(label);
    KernelIoWriteHex64(value);
    KernelIoWriteString(" ");
}

static int64_t Complete(OrynSysCallPacket* packet, int64_t status)
{
    packet->Status = status;
    gSysCallState.LastStatus = status;
    return status;
}

void OrynSysCallInit(void)
{
    ClearBytes(&gSysCallState, sizeof(gSysCallState));
    gSysCallState.Initialized = 1U;
    gSysCallState.PacketSize = (uint32_t)sizeof(OrynSysCallPacket);
}

const OrynSysCallState* OrynSysCallGetState(void)
{
    return &gSysCallState;
}

void OrynSysCallPreparePacket(
    OrynSysCallPacket* packet,
    uint64_t kind,
    uint64_t name_space,
    uint64_t operation)
{
    if (packet == 0)
    {
        return;
    }

    ClearBytes(packet, sizeof(*packet));
    packet->Magic = ORYN_SYSCALL_PACKET_MAGIC;
    packet->Version = ORYN_SYSCALL_PACKET_VERSION;
    packet->Size = sizeof(*packet);
    packet->Platform = ORYN_SYSCALL_PLATFORM_INTERNAL;
    packet->Kind = kind;
    packet->Namespace = name_space;
    packet->Operation = operation;
    packet->Status = ORYN_SYSCALL_STATUS_UNKNOWN;
}

void OrynSysCallNoteUnknownPlatform(uint64_t platform, uint64_t platform_number)
{
    gSysCallState.UnknownPackets += 1ULL;
    if (platform == ORYN_SYSCALL_PLATFORM_LINUX)
    {
        gSysCallState.UnknownLinuxPackets += 1ULL;
    }
    else if (platform == ORYN_SYSCALL_PLATFORM_MS)
    {
        gSysCallState.UnknownMsPackets += 1ULL;
    }

    KernelIoWriteString("[KERNEL] DEBUG: Unknown platform syscall platform=");
    KernelIoWriteHex64(platform);
    KernelIoWriteString(" number=");
    KernelIoWriteHex64(platform_number);
    KernelIoWriteString(" translated to SysCallEvent debug packet.\n");
}

void OrynSysCallPrintUnknown(const OrynSysCallPacket* packet)
{
    if (packet == 0)
    {
        KernelIoWriteString("[KERNEL] DEBUG: Unknown SysCall packet pointer was null.\n");
        return;
    }

    gSysCallState.UnknownPackets += 1ULL;
    KernelIoWriteString("[KERNEL] DEBUG: Unknown SysCall packet ");
    WritePacketField("platform=", packet->Platform);
    WritePacketField("number=", packet->PlatformNumber);
    WritePacketField("kind=", packet->Kind);
    WritePacketField("ns=", packet->Namespace);
    WritePacketField("op=", packet->Operation);
    KernelIoWriteString("\n");
}

int64_t SysCallGet(OrynSysCallPacket* packet)
{
    if (!IsValidPacket(packet))
    {
        return ORYN_SYSCALL_STATUS_BAD_PACKET;
    }

    gSysCallState.GetPackets += 1ULL;
    RecordPacket(packet);
    if (packet->Namespace == ORYN_SYSCALL_NS_KERNEL &&
        packet->Operation == ORYN_SYSCALL_OP_KERNEL_VERSION)
    {
        packet->Results[0] = 0ULL;
        packet->Results[1] = 5ULL;
        packet->Results[2] = 32ULL;
        return Complete(packet, ORYN_SYSCALL_STATUS_OK);
    }

    if (packet->Namespace == ORYN_SYSCALL_NS_PROCESS &&
        packet->Operation == ORYN_SYSCALL_OP_PROCESS_GET_ID)
    {
        packet->Results[0] = 1ULL;
        return Complete(packet, ORYN_SYSCALL_STATUS_OK);
    }

    OrynSysCallPrintUnknown(packet);
    return Complete(packet, ORYN_SYSCALL_STATUS_UNKNOWN);
}

int64_t SysCallSet(OrynSysCallPacket* packet)
{
    if (!IsValidPacket(packet))
    {
        return ORYN_SYSCALL_STATUS_BAD_PACKET;
    }

    gSysCallState.SetPackets += 1ULL;
    RecordPacket(packet);
    if (packet->Namespace == ORYN_SYSCALL_NS_DEBUG &&
        packet->Operation == ORYN_SYSCALL_OP_DEBUG_SET_LEVEL)
    {
        packet->Results[0] = packet->Arguments[0];
        KernelIoWriteString("[KERNEL] SysCallSet: debug level packet accepted.\n");
        return Complete(packet, ORYN_SYSCALL_STATUS_OK);
    }

    OrynSysCallPrintUnknown(packet);
    return Complete(packet, ORYN_SYSCALL_STATUS_UNKNOWN);
}

int64_t SysCallEvent(OrynSysCallPacket* packet)
{
    const char* text;

    if (!IsValidPacket(packet))
    {
        return ORYN_SYSCALL_STATUS_BAD_PACKET;
    }

    gSysCallState.EventPackets += 1ULL;
    RecordPacket(packet);
    if (packet->Namespace == ORYN_SYSCALL_NS_DEBUG &&
        packet->Operation == ORYN_SYSCALL_OP_DEBUG_WRITE_STRING)
    {
        text = packet->DebugText != 0 ? packet->DebugText : (const char*)packet->Arguments[1];
        if (text != 0)
        {
            KernelIoWriteString(text);
        }
        return Complete(packet, ORYN_SYSCALL_STATUS_OK);
    }

    if (packet->Namespace == ORYN_SYSCALL_NS_PROCESS &&
        packet->Operation == ORYN_SYSCALL_OP_PROCESS_EXIT)
    {
        KernelIoWriteString("[KERNEL] SysCallEvent: process exit event code ");
        KernelIoWriteDec64(packet->Arguments[0]);
        KernelIoWriteString(" received.\n");
        return Complete(packet, ORYN_SYSCALL_STATUS_OK);
    }

    if (packet->Namespace == ORYN_SYSCALL_NS_COMPAT &&
        packet->Operation == ORYN_SYSCALL_OP_COMPAT_UNKNOWN_PLATFORM_SYSCALL)
    {
        OrynSysCallNoteUnknownPlatform(packet->Platform, packet->PlatformNumber);
        return Complete(packet, ORYN_SYSCALL_STATUS_UNKNOWN);
    }

    OrynSysCallPrintUnknown(packet);
    return Complete(packet, ORYN_SYSCALL_STATUS_UNKNOWN);
}

int64_t OrynSysCallDispatch(OrynSysCallPacket* packet)
{
    if (!IsValidPacket(packet))
    {
        return ORYN_SYSCALL_STATUS_BAD_PACKET;
    }

    if (packet->Kind == ORYN_SYSCALL_KIND_GET)
    {
        return SysCallGet(packet);
    }

    if (packet->Kind == ORYN_SYSCALL_KIND_SET)
    {
        return SysCallSet(packet);
    }

    if (packet->Kind == ORYN_SYSCALL_KIND_EVENT)
    {
        return SysCallEvent(packet);
    }

    OrynSysCallPrintUnknown(packet);
    return Complete(packet, ORYN_SYSCALL_STATUS_UNKNOWN);
}

int OrynSysCallRunInternalProof(void)
{
    OrynSysCallPacket packet;

    gSysCallState.InternalProofRan = 1U;
    OrynSysCallPreparePacket(&packet, ORYN_SYSCALL_KIND_GET,
        ORYN_SYSCALL_NS_KERNEL, ORYN_SYSCALL_OP_KERNEL_VERSION);
    gSysCallState.GetProofPassed = SysCallGet(&packet) == ORYN_SYSCALL_STATUS_OK ? 1U : 0U;

    OrynSysCallPreparePacket(&packet, ORYN_SYSCALL_KIND_SET,
        ORYN_SYSCALL_NS_DEBUG, ORYN_SYSCALL_OP_DEBUG_SET_LEVEL);
    packet.Arguments[0] = 1ULL;
    gSysCallState.SetProofPassed = SysCallSet(&packet) == ORYN_SYSCALL_STATUS_OK ? 1U : 0U;

    OrynSysCallPreparePacket(&packet, ORYN_SYSCALL_KIND_EVENT,
        ORYN_SYSCALL_NS_DEBUG, ORYN_SYSCALL_OP_DEBUG_WRITE_STRING);
    packet.DebugText = "[KERNEL] SysCallEvent: debug message packet accepted.\n";
    gSysCallState.EventProofPassed = SysCallEvent(&packet) == ORYN_SYSCALL_STATUS_OK ? 1U : 0U;

    OrynSysCallPreparePacket(&packet, ORYN_SYSCALL_KIND_GET,
        ORYN_SYSCALL_NS_KERNEL, 0xFFFFFFFFULL);
    gSysCallState.UnknownProofPassed = SysCallGet(&packet) == ORYN_SYSCALL_STATUS_UNKNOWN ? 1U : 0U;

    return gSysCallState.GetProofPassed &&
        gSysCallState.SetProofPassed &&
        gSysCallState.EventProofPassed &&
        gSysCallState.UnknownProofPassed;
}

void OrynSysCallPrintProof(void)
{
    OrynKernelScreenReportOkOrFail(gSysCallState.Initialized,
        "SysCall core initialized.",
        "SysCall core not initialized.");
    OrynKernelScreenReportOkOrFail(gSysCallState.PacketSize == sizeof(OrynSysCallPacket),
        "SysCall message packet ABI ready.",
        "SysCall message packet ABI size mismatch.");
    KernelIoWriteString("[KERNEL] LinuxSysCall.h listed syscall count: ");
    KernelIoWriteDec64(LinuxSysCallListedCount());
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] LinuxSysCall.h translated syscall count: ");
    KernelIoWriteDec64(LinuxSysCallTranslatedCount());
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] MSSysCall.h listed syscall count: ");
    KernelIoWriteDec64(MSSysCallListedCount());
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] MSSysCall.h translated syscall count: ");
    KernelIoWriteDec64(MSSysCallTranslatedCount());
    KernelIoWriteString("\n");
    OrynKernelScreenReportOkOrFail(LinuxSysCallListedCount() == ORYN_LINUX_SYSCALL_LISTED_COUNT && MSSysCallListedCount() == ORYN_MS_SYSCALL_LISTED_COUNT,
        "SysCall header counts listed.",
        "SysCall header counts missing.");
}

void OrynSysCallPrintRuntimeProof(void)
{
    OrynKernelScreenReportOkOrFail(gSysCallState.GetProofPassed,
        "SysCallGet packet handled.",
        "SysCallGet packet was not handled.");
    OrynKernelScreenReportOkOrFail(gSysCallState.SetProofPassed,
        "SysCallSet packet handled.",
        "SysCallSet packet was not handled.");
    OrynKernelScreenReportOkOrFail(gSysCallState.EventProofPassed,
        "SysCallEvent packet handled.",
        "SysCallEvent packet was not handled.");
    OrynKernelScreenReportOkOrFail(gSysCallState.UnknownProofPassed && gSysCallState.UnknownPackets != 0ULL,
        "Unknown syscall debug logging path executed.",
        "Unknown syscall debug logging path did not execute.");
    OrynKernelScreenReportOkOrFail(gSysCallState.GetProofPassed && gSysCallState.SetProofPassed && gSysCallState.EventProofPassed,
        "SysCalls use Get/Set/Event message packets.",
        "SysCalls did not prove Get/Set/Event packets.");
    KernelIoWriteString("[KERNEL] SysCall total packets: ");
    KernelIoWriteDec64(gSysCallState.TotalPackets);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] SysCall unknown packets: ");
    KernelIoWriteDec64(gSysCallState.UnknownPackets);
    KernelIoWriteString("\n");
}
