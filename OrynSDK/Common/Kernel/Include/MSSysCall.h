#ifndef ORYN_MS_SYSCALL_H
#define ORYN_MS_SYSCALL_H

#include "SysCall.h"

#define ORYN_MS_SYSCALL_DEBUG_PRINT 0x1000ULL
#define ORYN_MS_SYSCALL_GET_VERSION 0x1001ULL
#define ORYN_MS_SYSCALL_PROCESS_EXIT 0x1002ULL
#define ORYN_MS_SYSCALL_NT_WRITE_FILE_COMPAT 0x0008ULL
#define ORYN_MS_SYSCALL_NT_CLOSE_COMPAT 0x000FULL
#define ORYN_MS_SYSCALL_NT_QUERY_INFORMATION_PROCESS_COMPAT 0x0022ULL

#define ORYN_MS_SYSCALL_LISTED_COUNT 6ULL
#define ORYN_MS_SYSCALL_TRANSLATED_COUNT 4ULL
#define ORYN_MS_SYSCALL_UNKNOWN_DEBUG_COUNT     (ORYN_MS_SYSCALL_LISTED_COUNT - ORYN_MS_SYSCALL_TRANSLATED_COUNT)

static inline uint64_t MSSysCallListedCount(void)
{
    return ORYN_MS_SYSCALL_LISTED_COUNT;
}

static inline uint64_t MSSysCallTranslatedCount(void)
{
    return ORYN_MS_SYSCALL_TRANSLATED_COUNT;
}

int MSSysCallTranslate(
    uint64_t number,
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    OrynSysCallPacket* packet);
int64_t MSSysCallDispatch(
    uint64_t number,
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5);

#endif
