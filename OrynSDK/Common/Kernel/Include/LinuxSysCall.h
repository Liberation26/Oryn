#ifndef ORYN_LINUX_SYSCALL_H
#define ORYN_LINUX_SYSCALL_H

#include "SysCall.h"

#define ORYN_LINUX_SYSCALL_READ 0ULL
#define ORYN_LINUX_SYSCALL_WRITE 1ULL
#define ORYN_LINUX_SYSCALL_OPEN 2ULL
#define ORYN_LINUX_SYSCALL_CLOSE 3ULL
#define ORYN_LINUX_SYSCALL_MMAP 9ULL
#define ORYN_LINUX_SYSCALL_MUNMAP 11ULL
#define ORYN_LINUX_SYSCALL_BRK 12ULL
#define ORYN_LINUX_SYSCALL_GETPID 39ULL
#define ORYN_LINUX_SYSCALL_UNAME 63ULL
#define ORYN_LINUX_SYSCALL_EXIT 60ULL
#define ORYN_LINUX_SYSCALL_EXIT_GROUP 231ULL

int LinuxSysCallTranslate(
    uint64_t number,
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    OrynSysCallPacket* packet);
int64_t LinuxSysCallDispatch(
    uint64_t number,
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5);

#endif
