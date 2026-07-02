#ifndef ORYN_KERNEL_USER_COMMAND_H
#define ORYN_KERNEL_USER_COMMAND_H

#include "KernelUserExecutable.h"

#define ORYN_USER_COMMAND_MAX_ARGS 8U
#define ORYN_USER_COMMAND_MAX_ENVS 8U
#define ORYN_USER_COMMAND_ARG_BYTES 256U
#define ORYN_USER_COMMAND_ENV_BYTES 256U
#define ORYN_USER_COMMAND_HANDLE_STDIN 0U
#define ORYN_USER_COMMAND_HANDLE_STDOUT 1U
#define ORYN_USER_COMMAND_HANDLE_STDERR 2U
#define ORYN_USER_COMMAND_MAX_EXTERNALS 16U
#define ORYN_USER_COMMAND_PERMISSION_READ 1ULL
#define ORYN_USER_COMMAND_PERMISSION_WRITE 2ULL
#define ORYN_USER_COMMAND_PERMISSION_EXECUTE 4ULL
#define ORYN_USER_COMMAND_PERMISSION_USER 8ULL
#define ORYN_USER_COMMAND_PATH_BYTES 128U

typedef enum OrynUserCommandPersonality
{
    OrynUserCommandPersonalityNative = 0,
    OrynUserCommandPersonalityLinux = 1,
    OrynUserCommandPersonalityWindows = 2,
    OrynUserCommandPersonalityAmiga = 3
} OrynUserCommandPersonality;

typedef struct OrynUserCommandDescriptor
{
    unsigned long long AbiVersion;
    unsigned long long Entry;
    unsigned long long StackTop;
    unsigned long long Permissions;
    unsigned int Argc;
    unsigned int Envc;
    unsigned int StdinHandle;
    unsigned int StdoutHandle;
    unsigned int StderrHandle;
    unsigned int Personality;
    char StoredPath[ORYN_USER_COMMAND_PATH_BYTES];
    char VisiblePath[ORYN_USER_COMMAND_PATH_BYTES];
    char Arguments[ORYN_USER_COMMAND_ARG_BYTES];
    char Environment[ORYN_USER_COMMAND_ENV_BYTES];
} OrynUserCommandDescriptor;

typedef struct OrynUserCommandState
{
    unsigned int PathTraversalRejectedCount;
    unsigned int SlashNameRejectedCount;
    unsigned int DescriptorCreatedCount;
    unsigned int ArgvEnvpCreatedCount;
    unsigned int StandardHandleReadyCount;
    unsigned int ExecutablePermissionReadyCount;
    unsigned int StaticProgramPolicyReadyCount;
    unsigned int SharedLibraryRejectedCount;
    unsigned int ExternalCommandConvertedCount;
    unsigned int HelpCommandExternalizedCount;
    unsigned int ShellApplicationReadyCount;
    unsigned int PersonalityVisiblePathReadyCount;
    unsigned int NativeVisiblePathReadyCount;
    unsigned int LinuxVisiblePathReadyCount;
    unsigned int WindowsVisiblePathReadyCount;
    unsigned int AmigaVisiblePathReadyCount;
    unsigned int PhysicalCommandStoreReadyCount;
} OrynUserCommandState;

void OrynUserCommandInit(void);
int OrynUserCommandNameIsSafe(const char* name);
int OrynUserCommandPathIsAllowed(const char* path, const char* root);
int OrynUserCommandResolveName(const char* name, const char* root, char* output, unsigned int capacity);
int OrynUserCommandResolveVisiblePath(unsigned int personality,
    const char* name,
    char* output,
    unsigned int capacity);
int OrynUserCommandSharedLibrariesAllowed(void);
int OrynUserCommandRecordExternalCommand(const char* name);
int OrynUserCommandRecordHelpCommand(const char* path);
int OrynUserCommandRecordShellApplication(const char* path);
int OrynUserCommandBuildDescriptor(const OrynUserExecutableImage* image,
    const char* const* argv,
    unsigned int argc,
    const char* const* envp,
    unsigned int envc,
    unsigned int personality,
    const char* storedPath,
    OrynUserCommandDescriptor* descriptor);
const OrynUserCommandState* OrynUserCommandGetState(void);
int OrynUserCommandRunSelfTest(const OrynUserExecutableImage* image, const char* root);
void OrynUserCommandPrintProof(void);

#endif
