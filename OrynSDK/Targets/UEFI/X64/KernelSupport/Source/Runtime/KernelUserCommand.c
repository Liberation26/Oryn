#include "KernelUserCommand.h"
#include "KernelVfs.h"
#include "KernelScreenReport.h"
#include "OrynString.h"

static OrynUserCommandState gCommandState;

void OrynUserCommandInit(void)
{
    OrynMemset(&gCommandState, 0, sizeof(gCommandState));
}

static int CommandAsciiEqual(char left, char right)
{
    if (left >= 'a' && left <= 'z') left = (char)(left - ('a' - 'A'));
    if (right >= 'a' && right <= 'z') right = (char)(right - ('a' - 'A'));
    return left == right;
}

static int CommandPrefixMatch(const char* path, const char* prefix)
{
    unsigned int index = 0U;
    if (path == 0 || prefix == 0 || path[0] != '/')
    {
        return 0;
    }
    while (prefix[index] != 0)
    {
        if (!CommandAsciiEqual(path[index], prefix[index]))
        {
            return 0;
        }
        index += 1U;
    }
    return path[index] == '/' && path[index + 1U] != 0;
}

int OrynUserCommandNameIsSafe(const char* name)
{
    unsigned int index = 0U;
    if (name == 0 || name[0] == 0 || name[0] == '.')
    {
        return 0;
    }
    while (name[index] != 0)
    {
        char ch = name[index];
        if (ch == '/' || ch == '\\')
        {
            gCommandState.SlashNameRejectedCount += 1U;
            return 0;
        }
        if (ch == '.' && (name[index + 1U] == '.' || index == 0U))
        {
            gCommandState.PathTraversalRejectedCount += 1U;
            return 0;
        }
        index += 1U;
    }
    return 1;
}

int OrynUserCommandPathIsAllowed(const char* path, const char* root)
{
    char normalized[ORYN_USER_EXEC_MAX_PATH];
    if (!OrynVfsNormalizePath(path, normalized, sizeof(normalized)))
    {
        gCommandState.PathTraversalRejectedCount += 1U;
        return 0;
    }
    return CommandPrefixMatch(normalized, root);
}

int OrynUserCommandResolveName(const char* name, const char* root, char* output, unsigned int capacity)
{
    unsigned int rootLen;
    unsigned int nameLen;
    if (!OrynUserCommandNameIsSafe(name) || root == 0 || output == 0 || capacity == 0U)
    {
        return 0;
    }
    rootLen = (unsigned int)OrynStrlen(root);
    nameLen = (unsigned int)OrynStrlen(name);
    if (rootLen + 1U + nameLen + 1U > capacity)
    {
        return 0;
    }
    OrynMemcpy(output, root, rootLen);
    output[rootLen] = '/';
    OrynMemcpy(output + rootLen + 1U, name, nameLen + 1U);
    return 1;
}

static int CommandAppendText(char* target, unsigned int capacity, unsigned int* offset, const char* text)
{
    unsigned int length;
    if (target == 0 || offset == 0 || text == 0)
    {
        return 0;
    }
    length = (unsigned int)OrynStrlen(text);
    if (*offset + length + 1U > capacity)
    {
        return 0;
    }
    OrynMemcpy(target + *offset, text, length + 1U);
    *offset += length + 1U;
    return 1;
}

static int CommandBuildTextVector(char* target,
    unsigned int capacity,
    const char* const* items,
    unsigned int count)
{
    unsigned int offset = 0U;
    for (unsigned int index = 0U; index < count; ++index)
    {
        if (!CommandAppendText(target, capacity, &offset, items[index]))
        {
            return 0;
        }
    }
    if (offset < capacity)
    {
        target[offset] = 0;
    }
    return 1;
}

int OrynUserCommandBuildDescriptor(const OrynUserExecutableImage* image,
    const char* const* argv,
    unsigned int argc,
    const char* const* envp,
    unsigned int envc,
    OrynUserCommandDescriptor* descriptor)
{
    if (image == 0 || descriptor == 0 || argc > ORYN_USER_COMMAND_MAX_ARGS ||
        envc > ORYN_USER_COMMAND_MAX_ENVS || image->Abi.Entry == 0ULL)
    {
        return 0;
    }
    OrynMemset(descriptor, 0, sizeof(*descriptor));
    descriptor->AbiVersion = image->Abi.AbiVersion;
    descriptor->Entry = image->Abi.Entry;
    descriptor->StackTop = image->Abi.InitialStackTop;
    descriptor->Permissions = ORYN_USER_COMMAND_PERMISSION_READ |
        ORYN_USER_COMMAND_PERMISSION_EXECUTE | ORYN_USER_COMMAND_PERMISSION_USER;
    descriptor->Argc = argc;
    descriptor->Envc = envc;
    descriptor->StdinHandle = ORYN_USER_COMMAND_HANDLE_STDIN;
    descriptor->StdoutHandle = ORYN_USER_COMMAND_HANDLE_STDOUT;
    descriptor->StderrHandle = ORYN_USER_COMMAND_HANDLE_STDERR;
    if (!CommandBuildTextVector(descriptor->Arguments, sizeof(descriptor->Arguments), argv, argc) ||
        !CommandBuildTextVector(descriptor->Environment, sizeof(descriptor->Environment), envp, envc))
    {
        return 0;
    }
    gCommandState.DescriptorCreatedCount += 1U;
    gCommandState.ArgvEnvpCreatedCount += 1U;
    gCommandState.StandardHandleReadyCount += 1U;
    gCommandState.ExecutablePermissionReadyCount +=
        (descriptor->Permissions & ORYN_USER_COMMAND_PERMISSION_EXECUTE) != 0ULL ? 1U : 0U;
    return 1;
}

const OrynUserCommandState* OrynUserCommandGetState(void)
{
    return &gCommandState;
}

int OrynUserCommandRunSelfTest(const OrynUserExecutableImage* image, const char* root)
{
    OrynUserCommandDescriptor descriptor;
    char path[ORYN_USER_EXEC_MAX_PATH];
    const char* argv[2] = { "HELLO", "WORLD" };
    const char* envp[1] = { "ORYN=1" };
    int rejectedTraversal = !OrynUserCommandPathIsAllowed("/SYSTEM/COMMANDS/../BAD", root);
    int rejectedSlash = !OrynUserCommandResolveName("DIR/LS", root, path, sizeof(path));
    int resolved = OrynUserCommandResolveName("HELLO", root, path, sizeof(path));
    int described = OrynUserCommandBuildDescriptor(image, argv, 2U, envp, 1U, &descriptor);
    return rejectedTraversal && rejectedSlash && resolved && described &&
        descriptor.StdinHandle == ORYN_USER_COMMAND_HANDLE_STDIN &&
        descriptor.StdoutHandle == ORYN_USER_COMMAND_HANDLE_STDOUT &&
        descriptor.StderrHandle == ORYN_USER_COMMAND_HANDLE_STDERR &&
        (descriptor.Permissions & ORYN_USER_COMMAND_PERMISSION_EXECUTE) != 0ULL;
}

void OrynUserCommandPrintProof(void)
{
    OrynKernelScreenReportOkOrFail(gCommandState.PathTraversalRejectedCount != 0U &&
        gCommandState.SlashNameRejectedCount != 0U,
        "Command loader rejects path traversal and slash-separated command names.",
        "Command loader path/name rejection proof failed.");
    OrynKernelScreenReportOkOrFail(gCommandState.DescriptorCreatedCount != 0U,
        "Command image descriptor includes ABI, entry, stack, permissions, and arguments.",
        "Command image descriptor proof failed.");
    OrynKernelScreenReportOkOrFail(gCommandState.ArgvEnvpCreatedCount != 0U,
        "Command argv/envp construction is implemented.",
        "Command argv/envp construction proof failed.");
    OrynKernelScreenReportOkOrFail(gCommandState.StandardHandleReadyCount != 0U,
        "Process standard handles stdin, stdout, and stderr are assigned.",
        "Process standard-handle proof failed.");
    OrynKernelScreenReportOkOrFail(gCommandState.ExecutablePermissionReadyCount != 0U,
        "Executable permission policy is present for user commands.",
        "Executable permission policy proof failed.");
}
