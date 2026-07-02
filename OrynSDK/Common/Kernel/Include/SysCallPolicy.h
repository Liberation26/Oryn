#ifndef ORYN_SYSCALL_POLICY_H
#define ORYN_SYSCALL_POLICY_H

#include "SysCall.h"

#define ORYN_SYSCALL_ABI_MIN_VERSION 1ULL
#define ORYN_SYSCALL_ABI_MAX_VERSION 1ULL
#define ORYN_SYSCALL_USER_LOW 0x0000000000010000ULL
#define ORYN_SYSCALL_USER_HIGH 0x00007FFFFFFFFFFFULL
#define ORYN_SYSCALL_TRACE_DEBUG 1U

#define ORYN_SYSCALL_ERRNO_EINVAL 22LL
#define ORYN_SYSCALL_ERRNO_ENOSYS 38LL
#define ORYN_SYSCALL_ERRNO_EFAULT 14LL
#define ORYN_SYSCALL_ERRNO_EPERM 1LL

#define ORYN_SYSCALL_CRED_KERNEL_UID 0ULL
#define ORYN_SYSCALL_CRED_KERNEL_GID 0ULL
#define ORYN_SYSCALL_CRED_USER_UID 1000ULL
#define ORYN_SYSCALL_CRED_USER_GID 1000ULL

void OrynSysCallPolicyInit(OrynSysCallState* state);
int OrynSysCallValidatePacket(const OrynSysCallPacket* packet, OrynSysCallState* state);
int OrynSysCallValidateUserPointer(uint64_t address, uint64_t size, OrynSysCallState* state);
int OrynSysCallValidateArguments(const OrynSysCallPacket* packet, OrynSysCallState* state);
int64_t OrynSysCallMapStatusToErrno(int64_t status);
void OrynSysCallTracePacket(const OrynSysCallPacket* packet, OrynSysCallState* state);
void OrynSysCallInstallCredentialPlaceholder(OrynSysCallPacket* packet, OrynSysCallState* state);
int OrynSysCallPolicyRouteUserRequest(
    uint64_t platform,
    uint64_t platform_number,
    const uint64_t* arguments,
    uint64_t argument_count,
    OrynSysCallPacket* packet,
    OrynSysCallState* state);
int OrynSysCallRunFuzzTests(OrynSysCallState* state);
void OrynSysCallPrintPolicyProof(const OrynSysCallState* state);

#endif
