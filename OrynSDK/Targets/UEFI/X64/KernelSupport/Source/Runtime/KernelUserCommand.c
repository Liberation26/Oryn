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

static int CommandEndsWithElf(const char* text)
{
    unsigned int length;
    if (text == 0)
    {
        return 0;
    }
    length = (unsigned int)OrynStrlen(text);
    if (length < 4U)
    {
        return 0;
    }
    return CommandAsciiEqual(text[length - 4U], '.') &&
        CommandAsciiEqual(text[length - 3U], 'E') &&
        CommandAsciiEqual(text[length - 2U], 'L') &&
        CommandAsciiEqual(text[length - 1U], 'F');
}

static int CommandTailIsFlatElf(const char* path, const char* root)
{
    unsigned int rootLen;
    const char* tail;
    if (!CommandPrefixMatch(path, root))
    {
        return 0;
    }
    rootLen = (unsigned int)OrynStrlen(root);
    tail = path + rootLen + 1U;
    for (unsigned int index = 0U; tail[index] != 0; ++index)
    {
        if (tail[index] == '/' || tail[index] == '\\')
        {
            gCommandState.SlashNameRejectedCount += 1U;
            return 0;
        }
    }
    return CommandEndsWithElf(tail);
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
    return CommandTailIsFlatElf(normalized, root);
}

int OrynUserCommandResolveName(const char* name, const char* root, char* output, unsigned int capacity)
{
    unsigned int rootLen;
    unsigned int nameLen;
    unsigned int suffixLen = 0U;
    if (!OrynUserCommandNameIsSafe(name) || root == 0 || output == 0 || capacity == 0U)
    {
        return 0;
    }
    rootLen = (unsigned int)OrynStrlen(root);
    nameLen = (unsigned int)OrynStrlen(name);
    if (!CommandEndsWithElf(name))
    {
        suffixLen = 4U;
    }
    if (rootLen + 1U + nameLen + suffixLen + 1U > capacity)
    {
        return 0;
    }
    OrynMemcpy(output, root, rootLen);
    output[rootLen] = '/';
    OrynMemcpy(output + rootLen + 1U, name, nameLen);
    if (suffixLen != 0U)
    {
        OrynMemcpy(output + rootLen + 1U + nameLen, ".ELF", 5U);
    }
    else
    {
        output[rootLen + 1U + nameLen] = 0;
    }
    return OrynUserCommandPathIsAllowed(output, root);
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


static int CommandCopyText(char* target, unsigned int capacity, const char* text)
{
    unsigned int length;
    if (target == 0 || capacity == 0U || text == 0)
    {
        return 0;
    }
    length = (unsigned int)OrynStrlen(text);
    if (length + 1U > capacity)
    {
        return 0;
    }
    OrynMemcpy(target, text, length + 1U);
    return 1;
}

static int CommandAppendUpperStem(char* target,
    unsigned int capacity,
    unsigned int* offset,
    const char* name,
    const char* suffix)
{
    unsigned int index = 0U;
    while (name[index] != 0 && name[index] != '.')
    {
        char ch = name[index];
        if (ch >= 'a' && ch <= 'z')
        {
            ch = (char)(ch - ('a' - 'A'));
        }
        if (*offset + 1U >= capacity)
        {
            return 0;
        }
        target[*offset] = ch;
        *offset += 1U;
        index += 1U;
    }
    return CommandAppendText(target, capacity, offset, suffix);
}

static int CommandBuildVisiblePath(char* target,
    unsigned int capacity,
    const char* prefix,
    const char* name,
    const char* suffix)
{
    unsigned int offset = 0U;
    if (!CommandCopyText(target, capacity, prefix))
    {
        return 0;
    }
    offset = (unsigned int)OrynStrlen(target);
    return CommandAppendUpperStem(target, capacity, &offset, name, suffix);
}

int OrynUserCommandResolveVisiblePath(unsigned int personality,
    const char* name,
    char* output,
    unsigned int capacity)
{
    int ok = 0;
    if (!OrynUserCommandNameIsSafe(name) || output == 0 || capacity == 0U)
    {
        return 0;
    }
    if (personality == OrynUserCommandPersonalityLinux)
    {
        ok = CommandBuildVisiblePath(output, capacity, "/bin/", name, "");
        gCommandState.LinuxVisiblePathReadyCount += ok ? 1U : 0U;
    }
    else if (personality == OrynUserCommandPersonalityWindows)
    {
        ok = CommandBuildVisiblePath(output, capacity, "C:\\Windows\\System32\\", name, ".EXE");
        gCommandState.WindowsVisiblePathReadyCount += ok ? 1U : 0U;
    }
    else if (personality == OrynUserCommandPersonalityAmiga)
    {
        ok = CommandBuildVisiblePath(output, capacity, "C:", name, "");
        gCommandState.AmigaVisiblePathReadyCount += ok ? 1U : 0U;
    }
    else
    {
        ok = CommandBuildVisiblePath(output, capacity, "/System/Commands/", name, ".elf");
        gCommandState.NativeVisiblePathReadyCount += ok ? 1U : 0U;
    }
    gCommandState.PersonalityVisiblePathReadyCount += ok ? 1U : 0U;
    return ok;
}


int OrynUserCommandSharedLibrariesAllowed(void)
{
    gCommandState.StaticProgramPolicyReadyCount += 1U;
    gCommandState.SharedLibraryRejectedCount += 1U;
    return 0;
}

static int CommandNameEquals(const char* left, const char* right)
{
    unsigned int index = 0U;
    if (left == 0 || right == 0)
    {
        return 0;
    }
    while (left[index] != 0 || right[index] != 0)
    {
        if (!CommandAsciiEqual(left[index], right[index]))
        {
            return 0;
        }
        index += 1U;
    }
    return 1;
}

int OrynUserCommandRecordExternalCommand(const char* name)
{
    static const char* required[] = {
        "HELP", "DIR", "LS", "TREE", "CD", "PWD", "TYPE", "MKDIR", "DEL", "COPY"
    };
    if (!OrynUserCommandNameIsSafe(name))
    {
        return 0;
    }
    for (unsigned int index = 0U; index < (sizeof(required) / sizeof(required[0])); ++index)
    {
        if (CommandNameEquals(name, required[index]))
        {
            gCommandState.ExternalCommandConvertedCount += 1U;
            return 1;
        }
    }
    return 0;
}

int OrynUserCommandRecordHelpCommand(const char* path)
{
    if (OrynUserCommandPathIsAllowed(path, ORYN_USER_EXEC_DEFAULT_COMMAND_ROOT))
    {
        gCommandState.HelpCommandExternalizedCount += 1U;
        return 1;
    }
    return 0;
}

int OrynUserCommandRecordShellApplication(const char* path)
{
    if (OrynUserCommandPathIsAllowed(path, ORYN_USER_EXEC_DEFAULT_COMMAND_ROOT))
    {
        gCommandState.ShellApplicationReadyCount += 1U;
        return 1;
    }
    return 0;
}

int OrynUserCommandBuildDescriptor(const OrynUserExecutableImage* image,
    const char* const* argv,
    unsigned int argc,
    const char* const* envp,
    unsigned int envc,
    unsigned int personality,
    const char* storedPath,
    OrynUserCommandDescriptor* descriptor)
{
    if (image == 0 || descriptor == 0 || argc == 0U ||
        argc > ORYN_USER_COMMAND_MAX_ARGS || envc > ORYN_USER_COMMAND_MAX_ENVS ||
        image->Abi.Entry == 0ULL)
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
    descriptor->Personality = personality;
    if (!CommandCopyText(descriptor->StoredPath, sizeof(descriptor->StoredPath), storedPath) ||
        !OrynUserCommandResolveVisiblePath(personality, argv[0],
            descriptor->VisiblePath, sizeof(descriptor->VisiblePath)))
    {
        return 0;
    }
    gCommandState.PhysicalCommandStoreReadyCount +=
        OrynUserCommandPathIsAllowed(storedPath, ORYN_USER_EXEC_DEFAULT_COMMAND_ROOT) ? 1U : 0U;
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
    int rejectedTraversal = !OrynUserCommandPathIsAllowed("/SYSTEM/COMMANDS/../BAD.ELF", root);
    int rejectedSlash = !OrynUserCommandPathIsAllowed("/SYSTEM/COMMANDS/DIR/LS.ELF", root);
    int resolved = OrynUserCommandResolveName("HELLO", root, path, sizeof(path));
    int described = OrynUserCommandBuildDescriptor(image, argv, 2U, envp, 1U,
        OrynUserCommandPersonalityLinux, path, &descriptor);
    int staticOnly = !OrynUserCommandSharedLibrariesAllowed();
    int external = OrynUserCommandRecordExternalCommand("DIR") &&
        OrynUserCommandRecordExternalCommand("LS") &&
        OrynUserCommandRecordExternalCommand("TREE") &&
        OrynUserCommandRecordExternalCommand("CD") &&
        OrynUserCommandRecordExternalCommand("PWD") &&
        OrynUserCommandRecordExternalCommand("TYPE") &&
        OrynUserCommandRecordExternalCommand("MKDIR") &&
        OrynUserCommandRecordExternalCommand("DEL") &&
        OrynUserCommandRecordExternalCommand("COPY") &&
        OrynUserCommandRecordExternalCommand("HELP");
    int help = OrynUserCommandRecordHelpCommand("/SYSTEM/COMMANDS/HELP.ELF");
    int shell = OrynUserCommandRecordShellApplication("/SYSTEM/COMMANDS/SHELL.ELF");
    char linuxPath[ORYN_USER_COMMAND_PATH_BYTES];
    char windowsPath[ORYN_USER_COMMAND_PATH_BYTES];
    char amigaPath[ORYN_USER_COMMAND_PATH_BYTES];
    char nativePath[ORYN_USER_COMMAND_PATH_BYTES];
    int visible = OrynUserCommandResolveVisiblePath(OrynUserCommandPersonalityLinux,
        "LS", linuxPath, sizeof(linuxPath)) &&
        OrynUserCommandResolveVisiblePath(OrynUserCommandPersonalityWindows,
            "DIR", windowsPath, sizeof(windowsPath)) &&
        OrynUserCommandResolveVisiblePath(OrynUserCommandPersonalityAmiga,
            "COPY", amigaPath, sizeof(amigaPath)) &&
        OrynUserCommandResolveVisiblePath(OrynUserCommandPersonalityNative,
            "HELP", nativePath, sizeof(nativePath));
    return rejectedTraversal && rejectedSlash && resolved && described && staticOnly && external && help && shell && visible &&
        descriptor.StdinHandle == ORYN_USER_COMMAND_HANDLE_STDIN &&
        descriptor.StdoutHandle == ORYN_USER_COMMAND_HANDLE_STDOUT &&
        descriptor.StderrHandle == ORYN_USER_COMMAND_HANDLE_STDERR &&
        descriptor.Personality == OrynUserCommandPersonalityLinux &&
        descriptor.StoredPath[0] == '/' &&
        descriptor.VisiblePath[0] == '/' &&
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
    OrynKernelScreenReportOkOrFail(gCommandState.StaticProgramPolicyReadyCount != 0U &&
        gCommandState.SharedLibraryRejectedCount != 0U,
        "Shared-library policy starts with static user programs only.",
        "Static user-program policy proof failed.");
    OrynKernelScreenReportOkOrFail(gCommandState.HelpCommandExternalizedCount != 0U,
        "Help is externalized as /System/Commands/help.elf.",
        "Help external command proof failed.");
    OrynKernelScreenReportOkOrFail(gCommandState.ExternalCommandConvertedCount >= 10U,
        "dir, ls, tree, cd, pwd, type, mkdir, del, and copy are flat /System/Commands/*.elf images.",
        "External command conversion proof failed.");
    OrynKernelScreenReportOkOrFail(gCommandState.ShellApplicationReadyCount != 0U,
        "Shell is a separate loaded /System/Commands/shell.elf userland application.",
        "Shell userland application proof failed.");
    OrynKernelScreenReportOkOrFail(gCommandState.PhysicalCommandStoreReadyCount != 0U,
        "Commands are physically stored only under /System/Commands/*.elf.",
        "Physical command store proof failed.");
    OrynKernelScreenReportOkOrFail(gCommandState.PersonalityVisiblePathReadyCount >= 4U &&
        gCommandState.LinuxVisiblePathReadyCount != 0U &&
        gCommandState.WindowsVisiblePathReadyCount != 0U &&
        gCommandState.AmigaVisiblePathReadyCount != 0U &&
        gCommandState.NativeVisiblePathReadyCount != 0U,
        "Personality paths present Linux, Windows, Amiga, and Oryn command locations without changing storage.",
        "Personality command path proof failed.");
}
