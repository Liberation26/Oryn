#ifndef ORYN_KERNEL_USER_EXECUTABLE_H
#define ORYN_KERNEL_USER_EXECUTABLE_H

#include "KernelProcess.h"
#include "KernelPhysicalMemory.h"

#define ORYN_USER_EXEC_ABI_MAGIC 0x4F52594E55414249ULL
#define ORYN_USER_EXEC_ABI_VERSION 1U
#define ORYN_USER_EXEC_ABI_MIN_VERSION 1U
#define ORYN_USER_EXEC_ABI_MAX_VERSION 1U
#define ORYN_USER_EXEC_ABI_ARCH_X86_64 0x8664U
#define ORYN_USER_EXEC_DEFAULT_COMMAND_ROOT "/SYSTEM/COMMANDS"
#define ORYN_USER_EXEC_MAX_IMAGE_BYTES 65536U
#define ORYN_USER_EXEC_MAX_PATH 128U
#define ORYN_USER_EXEC_FLAG_SHARED_LIBRARY 1ULL
#define ORYN_USER_EXEC_FLAG_STATIC_PROGRAM 2ULL

typedef enum OrynUserExecStatus
{
    OrynUserExecStatusOk = 0,
    OrynUserExecStatusInvalidArgument = 1,
    OrynUserExecStatusDeniedPath = 2,
    OrynUserExecStatusVfsReadFailed = 3,
    OrynUserExecStatusInvalidElf = 4,
    OrynUserExecStatusInvalidAbi = 5,
    OrynUserExecStatusMapFailed = 6,
    OrynUserExecStatusThreadFailed = 7
} OrynUserExecStatus;

typedef struct OrynUserExecutableAbi
{
    unsigned long long Magic;
    unsigned int AbiVersion;
    unsigned int Architecture;
    unsigned long long Entry;
    unsigned long long InitialStackTop;
    unsigned long long Flags;
} OrynUserExecutableAbi;

typedef struct OrynUserExecutableImage
{
    OrynUserExecutableAbi Abi;
    unsigned long long ImageBase;
    unsigned long long ImageBytes;
    unsigned int ProgramHeaderCount;
    unsigned int LoadSegmentCount;
} OrynUserExecutableImage;

typedef struct OrynUserExecutableState
{
    unsigned int Initialized;
    unsigned int AbiDefined;
    unsigned int AbiVersionReady;
    unsigned int Elf64LoaderReady;
    unsigned int VfsLoadAttemptCount;
    unsigned int VfsLoadSuccessCount;
    unsigned int CommandRootReady;
    unsigned int CommandPathDeniedCount;
    unsigned int ExternalCommandOnlyReady;
    unsigned int LoadedSegmentCount;
    unsigned int CreatedProcessCount;
    unsigned int CreatedThreadCount;
    unsigned int StaticProgramOnlyReady;
    unsigned int SharedLibraryDeniedCount;
    unsigned int ExternalCommandImageCount;
    unsigned int HelpCommandImageReady;
    unsigned int ShellApplicationLoaded;
    unsigned int LastStatus;
    char CommandRoot[ORYN_USER_EXEC_MAX_PATH];
} OrynUserExecutableState;

void OrynUserExecutableInit(void);
int OrynUserExecutableSetCommandRoot(const char* root);
const char* OrynUserExecutableGetCommandRoot(void);
int OrynUserExecutablePathAllowed(const char* path);
int OrynUserExecutableValidateAbi(const OrynUserExecutableAbi* abi);
int OrynUserExecutableValidateStaticPolicy(const OrynUserExecutableAbi* abi);
int OrynUserExecutableLoadElf64FromVfs(
    OrynKernelPhysicalMemory* physicalMemory,
    const char* path,
    OrynKernelUserProcess* userProcess,
    OrynUserExecutableImage* image);
OrynKernelThread* OrynUserExecutableLoadExternalCommand(
    OrynKernelPhysicalMemory* physicalMemory,
    const char* path,
    OrynKernelUserProcess* userProcess,
    OrynUserExecutableImage* image);
const OrynUserExecutableState* OrynUserExecutableGetState(void);
int OrynUserExecutableRunSelfTest(OrynKernelPhysicalMemory* physicalMemory);
void OrynUserExecutablePrintProof(void);

#endif
