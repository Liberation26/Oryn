#ifndef ORYN_SYSCALL_H
#define ORYN_SYSCALL_H

#include "OrynStdDef.h"

#define ORYN_SYSCALL_PACKET_MAGIC 0x4F52594E53594341ULL
#define ORYN_SYSCALL_PACKET_VERSION 1ULL
#define ORYN_SYSCALL_ARGUMENT_COUNT 6U
#define ORYN_SYSCALL_RESULT_COUNT 4U

#define ORYN_SYSCALL_PLATFORM_INTERNAL 0ULL
#define ORYN_SYSCALL_PLATFORM_LINUX 1ULL
#define ORYN_SYSCALL_PLATFORM_MS 2ULL

#define ORYN_SYSCALL_KIND_GET 1ULL
#define ORYN_SYSCALL_KIND_SET 2ULL
#define ORYN_SYSCALL_KIND_EVENT 3ULL

#define ORYN_SYSCALL_NS_KERNEL 1ULL
#define ORYN_SYSCALL_NS_DEBUG 2ULL
#define ORYN_SYSCALL_NS_IO 3ULL
#define ORYN_SYSCALL_NS_PROCESS 4ULL
#define ORYN_SYSCALL_NS_MEMORY 5ULL
#define ORYN_SYSCALL_NS_COMPAT 6ULL

#define ORYN_SYSCALL_OP_KERNEL_VERSION 1ULL
#define ORYN_SYSCALL_OP_KERNEL_PING 2ULL
#define ORYN_SYSCALL_OP_DEBUG_WRITE_STRING 10ULL
#define ORYN_SYSCALL_OP_DEBUG_SET_LEVEL 11ULL
#define ORYN_SYSCALL_OP_PROCESS_EXIT 20ULL
#define ORYN_SYSCALL_OP_PROCESS_GET_ID 21ULL
#define ORYN_SYSCALL_OP_COMPAT_UNKNOWN_PLATFORM_SYSCALL 100ULL

#define ORYN_SYSCALL_STATUS_OK 0LL
#define ORYN_SYSCALL_STATUS_BAD_PACKET -1LL
#define ORYN_SYSCALL_STATUS_UNKNOWN -2LL
#define ORYN_SYSCALL_STATUS_UNSUPPORTED -3LL

typedef struct OrynSysCallPacket
{
    uint64_t Magic;
    uint64_t Version;
    uint64_t Size;
    uint64_t Platform;
    uint64_t PlatformNumber;
    uint64_t Kind;
    uint64_t Namespace;
    uint64_t Operation;
    uint64_t Flags;
    uint64_t Arguments[ORYN_SYSCALL_ARGUMENT_COUNT];
    uint64_t Results[ORYN_SYSCALL_RESULT_COUNT];
    int64_t Status;
    const char* DebugText;
} OrynSysCallPacket;

typedef struct OrynSysCallState
{
    uint32_t Initialized;
    uint32_t PacketSize;
    uint64_t TotalPackets;
    uint64_t GetPackets;
    uint64_t SetPackets;
    uint64_t EventPackets;
    uint64_t LinuxPackets;
    uint64_t MsPackets;
    uint64_t UnknownPackets;
    uint64_t UnknownLinuxPackets;
    uint64_t UnknownMsPackets;
    uint64_t LastPlatform;
    uint64_t LastPlatformNumber;
    uint64_t LastKind;
    uint64_t LastNamespace;
    uint64_t LastOperation;
    int64_t LastStatus;
    uint32_t InternalProofRan;
    uint32_t GetProofPassed;
    uint32_t SetProofPassed;
    uint32_t EventProofPassed;
    uint32_t UnknownProofPassed;
} OrynSysCallState;

void OrynSysCallInit(void);
const OrynSysCallState* OrynSysCallGetState(void);
void OrynSysCallPreparePacket(
    OrynSysCallPacket* packet,
    uint64_t kind,
    uint64_t name_space,
    uint64_t operation);
int64_t SysCallGet(OrynSysCallPacket* packet);
int64_t SysCallSet(OrynSysCallPacket* packet);
int64_t SysCallEvent(OrynSysCallPacket* packet);
int64_t OrynSysCallDispatch(OrynSysCallPacket* packet);
void OrynSysCallNoteUnknownPlatform(uint64_t platform, uint64_t platform_number);
void OrynSysCallPrintUnknown(const OrynSysCallPacket* packet);
int OrynSysCallRunInternalProof(void);
void OrynSysCallPrintProof(void);
void OrynSysCallPrintRuntimeProof(void);

#endif
