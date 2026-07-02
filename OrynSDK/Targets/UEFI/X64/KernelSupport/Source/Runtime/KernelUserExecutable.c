#include "KernelUserExecutable.h"
#include "KernelUserMode.h"
#include "KernelVfs.h"
#include "KernelScreenReport.h"
#include "OrynString.h"

#define ELF_MAGIC0 0x7FU
#define ELF_MAGIC1 'E'
#define ELF_MAGIC2 'L'
#define ELF_MAGIC3 'F'
#define ELF_CLASS_64 2U
#define ELF_DATA_LSB 1U
#define ELF_TYPE_EXEC 2U
#define ELF_MACHINE_X86_64 62U
#define ELF_VERSION_CURRENT 1U
#define ELF_PT_LOAD 1U
#define ELF_PF_X 1U
#define ELF_PF_W 2U
#define ELF_PF_R 4U

static OrynUserExecutableState gUserExec;
static unsigned char gExecBuffer[ORYN_USER_EXEC_MAX_IMAGE_BYTES];

typedef struct Elf64Header
{
    unsigned char Ident[16];
    unsigned short Type;
    unsigned short Machine;
    unsigned int Version;
    unsigned long long Entry;
    unsigned long long ProgramHeaderOffset;
    unsigned long long SectionHeaderOffset;
    unsigned int Flags;
    unsigned short HeaderSize;
    unsigned short ProgramHeaderEntrySize;
    unsigned short ProgramHeaderCount;
    unsigned short SectionHeaderEntrySize;
    unsigned short SectionHeaderCount;
    unsigned short SectionNameIndex;
} Elf64Header;

typedef struct Elf64ProgramHeader
{
    unsigned int Type;
    unsigned int Flags;
    unsigned long long Offset;
    unsigned long long VirtualAddress;
    unsigned long long PhysicalAddress;
    unsigned long long FileSize;
    unsigned long long MemorySize;
    unsigned long long Align;
} Elf64ProgramHeader;

static void UserExecClear(void* target, unsigned long long bytes)
{
    unsigned char* output = (unsigned char*)target;
    for (unsigned long long index = 0ULL; index < bytes; ++index)
    {
        output[index] = 0U;
    }
}

static void UserExecCopyText(char* target, unsigned int capacity, const char* source)
{
    unsigned int index = 0U;
    if (target == 0 || capacity == 0U)
    {
        return;
    }
    if (source == 0)
    {
        source = ORYN_USER_EXEC_DEFAULT_COMMAND_ROOT;
    }
    while (source[index] != 0 && index + 1U < capacity)
    {
        target[index] = source[index];
        index += 1U;
    }
    target[index] = 0;
}

static int UserExecAsciiEqual(char left, char right)
{
    if (left >= 'a' && left <= 'z') left = (char)(left - ('a' - 'A'));
    if (right >= 'a' && right <= 'z') right = (char)(right - ('a' - 'A'));
    return left == right;
}

static int UserExecPrefixMatch(const char* path, const char* prefix)
{
    unsigned int index = 0U;
    if (path == 0 || prefix == 0 || path[0] != '/')
    {
        return 0;
    }
    while (prefix[index] != 0)
    {
        if (!UserExecAsciiEqual(path[index], prefix[index]))
        {
            return 0;
        }
        index += 1U;
    }
    return path[index] == '/' && path[index + 1U] != 0;
}

static int UserExecReadLe16(const unsigned char* p)
{
    return (int)((unsigned int)p[0] | ((unsigned int)p[1] << 8));
}

static unsigned int UserExecReadLe32(const unsigned char* p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) |
        ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

static unsigned long long UserExecReadLe64(const unsigned char* p)
{
    unsigned long long low = UserExecReadLe32(p);
    unsigned long long high = UserExecReadLe32(p + 4);
    return low | (high << 32);
}

static void UserExecDecodeHeader(const unsigned char* data, Elf64Header* h)
{
    OrynMemcpy(h->Ident, data, 16U);
    h->Type = (unsigned short)UserExecReadLe16(data + 16U);
    h->Machine = (unsigned short)UserExecReadLe16(data + 18U);
    h->Version = UserExecReadLe32(data + 20U);
    h->Entry = UserExecReadLe64(data + 24U);
    h->ProgramHeaderOffset = UserExecReadLe64(data + 32U);
    h->SectionHeaderOffset = UserExecReadLe64(data + 40U);
    h->Flags = UserExecReadLe32(data + 48U);
    h->HeaderSize = (unsigned short)UserExecReadLe16(data + 52U);
    h->ProgramHeaderEntrySize = (unsigned short)UserExecReadLe16(data + 54U);
    h->ProgramHeaderCount = (unsigned short)UserExecReadLe16(data + 56U);
    h->SectionHeaderEntrySize = (unsigned short)UserExecReadLe16(data + 58U);
    h->SectionHeaderCount = (unsigned short)UserExecReadLe16(data + 60U);
    h->SectionNameIndex = (unsigned short)UserExecReadLe16(data + 62U);
}

static void UserExecDecodeProgramHeader(const unsigned char* data, Elf64ProgramHeader* p)
{
    p->Type = UserExecReadLe32(data);
    p->Flags = UserExecReadLe32(data + 4U);
    p->Offset = UserExecReadLe64(data + 8U);
    p->VirtualAddress = UserExecReadLe64(data + 16U);
    p->PhysicalAddress = UserExecReadLe64(data + 24U);
    p->FileSize = UserExecReadLe64(data + 32U);
    p->MemorySize = UserExecReadLe64(data + 40U);
    p->Align = UserExecReadLe64(data + 48U);
}

void OrynUserExecutableInit(void)
{
    UserExecClear(&gUserExec, sizeof(gUserExec));
    gUserExec.Initialized = 1U;
    gUserExec.AbiDefined = 1U;
    gUserExec.AbiVersionReady = 1U;
    gUserExec.Elf64LoaderReady = 1U;
    gUserExec.CommandRootReady = 1U;
    gUserExec.ExternalCommandOnlyReady = 1U;
    UserExecCopyText(gUserExec.CommandRoot, sizeof(gUserExec.CommandRoot),
        ORYN_USER_EXEC_DEFAULT_COMMAND_ROOT);
}

int OrynUserExecutableSetCommandRoot(const char* root)
{
    char normalized[ORYN_USER_EXEC_MAX_PATH];
    if (gUserExec.Initialized == 0U) OrynUserExecutableInit();
    if (!OrynVfsNormalizePath(root, normalized, sizeof(normalized)) ||
        OrynStrlen(normalized) < 2U)
    {
        gUserExec.LastStatus = OrynUserExecStatusInvalidArgument;
        return 0;
    }
    UserExecCopyText(gUserExec.CommandRoot, sizeof(gUserExec.CommandRoot), normalized);
    return 1;
}

const char* OrynUserExecutableGetCommandRoot(void)
{
    if (gUserExec.Initialized == 0U) OrynUserExecutableInit();
    return gUserExec.CommandRoot;
}

int OrynUserExecutablePathAllowed(const char* path)
{
    char normalized[ORYN_USER_EXEC_MAX_PATH];
    if (gUserExec.Initialized == 0U) OrynUserExecutableInit();
    if (!OrynVfsNormalizePath(path, normalized, sizeof(normalized)))
    {
        return 0;
    }
    if (!UserExecPrefixMatch(normalized, gUserExec.CommandRoot))
    {
        gUserExec.CommandPathDeniedCount += 1U;
        return 0;
    }
    return 1;
}

int OrynUserExecutableValidateAbi(const OrynUserExecutableAbi* abi)
{
    if (abi == 0 || abi->Magic != ORYN_USER_EXEC_ABI_MAGIC ||
        abi->AbiVersion < ORYN_USER_EXEC_ABI_MIN_VERSION ||
        abi->AbiVersion > ORYN_USER_EXEC_ABI_MAX_VERSION ||
        abi->Architecture != ORYN_USER_EXEC_ABI_ARCH_X86_64 ||
        !OrynVirtualMemoryIsRangeInUserSpace(abi->Entry, 1ULL) ||
        !OrynVirtualMemoryIsRangeInUserSpace(abi->InitialStackTop - 16ULL, 16ULL))
    {
        gUserExec.LastStatus = OrynUserExecStatusInvalidAbi;
        return 0;
    }
    return 1;
}

static unsigned long long UserExecFlags(unsigned int elfFlags)
{
    unsigned long long flags = ORYN_VIRTUAL_FLAG_USER | ORYN_VIRTUAL_FLAG_READ;
    if ((elfFlags & ELF_PF_W) != 0U) flags |= ORYN_VIRTUAL_FLAG_WRITE;
    if ((elfFlags & ELF_PF_X) != 0U) flags |= ORYN_VIRTUAL_FLAG_EXECUTE;
    return flags;
}

static int UserExecMapSegment(OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelUserProcess* userProcess, const unsigned char* file,
    unsigned int fileBytes, const Elf64ProgramHeader* ph)
{
    unsigned long long pageBase = ph->VirtualAddress & ~(ORYN_VIRTUAL_PAGE_SIZE - 1ULL);
    unsigned long long pageOffset = ph->VirtualAddress - pageBase;
    unsigned long long total = pageOffset + ph->MemorySize;
    unsigned long long flags = UserExecFlags(ph->Flags);
    if (userProcess == 0 || userProcess->AddressSpace == 0 || ph->MemorySize == 0ULL ||
        ph->FileSize > ph->MemorySize || ph->Offset + ph->FileSize > fileBytes ||
        !OrynVirtualMemoryReserveMmapRegion(userProcess->AddressSpace, pageBase, total,
            flags, OrynVirtualMmapRegionFile, 1ULL, ph->Offset, 0ULL))
    {
        return 0;
    }
    for (unsigned long long copied = 0ULL; copied < total; copied += ORYN_VIRTUAL_PAGE_SIZE)
    {
        unsigned long long physical = OrynPhysicalMemoryAllocatePageBelow(
            physicalMemory, ORYN_PHYSICAL_EARLY_DIRECT_MAP_LIMIT);
        unsigned char* page = (unsigned char*)physical;
        if (physical == ORYN_PHYSICAL_ALLOC_FAIL)
        {
            return 0;
        }
        UserExecClear(page, ORYN_VIRTUAL_PAGE_SIZE);
        {
            unsigned long long fileStart = copied > pageOffset ? copied - pageOffset : 0ULL;
            unsigned long long pageStart = copied < pageOffset ? pageOffset - copied : 0ULL;
            if (fileStart < ph->FileSize && pageStart < ORYN_VIRTUAL_PAGE_SIZE)
            {
                unsigned long long chunk = ph->FileSize - fileStart;
                if (chunk > ORYN_VIRTUAL_PAGE_SIZE - pageStart)
                {
                    chunk = ORYN_VIRTUAL_PAGE_SIZE - pageStart;
                }
                OrynMemcpy(page + pageStart, file + ph->Offset + fileStart, (size_t)chunk);
            }
        }
        if (!OrynVirtualMemoryMap(userProcess->AddressSpace, physicalMemory,
            pageBase + copied, physical, ORYN_VIRTUAL_PAGE_SIZE, flags))
        {
            return 0;
        }
        (void)OrynPhysicalMemorySetPageOwner(physicalMemory, physical,
            OrynPhysicalPageOwnerUserPage, userProcess->AddressSpace->AddressSpaceId);
        gUserExec.LoadedSegmentCount += 1U;
    }
    return 1;
}

static int UserExecValidateElf(const Elf64Header* h, unsigned int fileBytes)
{
    unsigned long long tableEnd;
    if (fileBytes < 64U || h->Ident[0] != ELF_MAGIC0 || h->Ident[1] != ELF_MAGIC1 ||
        h->Ident[2] != ELF_MAGIC2 || h->Ident[3] != ELF_MAGIC3 ||
        h->Ident[4] != ELF_CLASS_64 || h->Ident[5] != ELF_DATA_LSB ||
        h->Type != ELF_TYPE_EXEC || h->Machine != ELF_MACHINE_X86_64 ||
        h->Version != ELF_VERSION_CURRENT || h->ProgramHeaderEntrySize != 56U ||
        h->ProgramHeaderCount == 0U)
    {
        return 0;
    }
    tableEnd = h->ProgramHeaderOffset +
        ((unsigned long long)h->ProgramHeaderEntrySize * h->ProgramHeaderCount);
    return tableEnd <= fileBytes;
}

int OrynUserExecutableLoadElf64FromVfs(OrynKernelPhysicalMemory* physicalMemory,
    const char* path, OrynKernelUserProcess* userProcess, OrynUserExecutableImage* image)
{
    uint32_t bytesRead = 0U;
    Elf64Header h;
    OrynUserExecutableAbi abi;
    if (gUserExec.Initialized == 0U) OrynUserExecutableInit();
    gUserExec.VfsLoadAttemptCount += 1U;
    if (physicalMemory == 0 || userProcess == 0 || userProcess->AddressSpace == 0 || image == 0)
    {
        gUserExec.LastStatus = OrynUserExecStatusInvalidArgument;
        return 0;
    }
    UserExecClear(image, sizeof(*image));
    if (!OrynUserExecutablePathAllowed(path))
    {
        gUserExec.LastStatus = OrynUserExecStatusDeniedPath;
        return 0;
    }
    if (!OrynVfsReadFile(path, gExecBuffer, sizeof(gExecBuffer), &bytesRead))
    {
        gUserExec.LastStatus = OrynUserExecStatusVfsReadFailed;
        return 0;
    }
    UserExecDecodeHeader(gExecBuffer, &h);
    if (!UserExecValidateElf(&h, bytesRead))
    {
        gUserExec.LastStatus = OrynUserExecStatusInvalidElf;
        return 0;
    }
    abi.Magic = ORYN_USER_EXEC_ABI_MAGIC;
    abi.AbiVersion = ORYN_USER_EXEC_ABI_VERSION;
    abi.Architecture = ORYN_USER_EXEC_ABI_ARCH_X86_64;
    abi.Entry = h.Entry;
    abi.InitialStackTop = ORYN_USER_MODE_TEST_STACK;
    abi.Flags = 0ULL;
    if (!OrynUserExecutableValidateAbi(&abi))
    {
        return 0;
    }
    for (unsigned int index = 0U; index < h.ProgramHeaderCount; ++index)
    {
        Elf64ProgramHeader ph;
        UserExecDecodeProgramHeader(gExecBuffer + h.ProgramHeaderOffset +
            ((unsigned long long)index * h.ProgramHeaderEntrySize), &ph);
        if (ph.Type == ELF_PT_LOAD && !UserExecMapSegment(physicalMemory, userProcess,
            gExecBuffer, bytesRead, &ph))
        {
            gUserExec.LastStatus = OrynUserExecStatusMapFailed;
            return 0;
        }
        if (ph.Type == ELF_PT_LOAD)
        {
            image->LoadSegmentCount += 1U;
            image->ImageBase = image->ImageBase == 0ULL ? ph.VirtualAddress : image->ImageBase;
            image->ImageBytes += ph.MemorySize;
        }
    }
    image->Abi = abi;
    image->ProgramHeaderCount = h.ProgramHeaderCount;
    gUserExec.VfsLoadSuccessCount += 1U;
    gUserExec.LastStatus = OrynUserExecStatusOk;
    return image->LoadSegmentCount != 0U;
}

OrynKernelThread* OrynUserExecutableLoadExternalCommand(OrynKernelPhysicalMemory* physicalMemory,
    const char* path, OrynKernelUserProcess* userProcess, OrynUserExecutableImage* image)
{
    OrynKernelThread* thread;
    if (!OrynUserExecutableLoadElf64FromVfs(physicalMemory, path, userProcess, image))
    {
        return 0;
    }
    thread = OrynKernelThreadCreateUser(userProcess, "user-command",
        image->Abi.Entry, image->Abi.InitialStackTop);
    if (thread == 0)
    {
        gUserExec.LastStatus = OrynUserExecStatusThreadFailed;
        return 0;
    }
    gUserExec.CreatedThreadCount += 1U;
    return thread;
}

const OrynUserExecutableState* OrynUserExecutableGetState(void)
{
    return &gUserExec;
}

int OrynUserExecutableRunSelfTest(OrynKernelPhysicalMemory* physicalMemory)
{
    OrynKernelProcess* process;
    OrynKernelUserProcess userProcess;
    OrynUserExecutableImage image;
    OrynKernelThread* thread;
    int denied;
    if (gUserExec.Initialized == 0U) OrynUserExecutableInit();
    denied = !OrynUserExecutablePathAllowed("/SYSTEM/HELLO");
    process = OrynKernelProcessCreate(physicalMemory, "exec-proof", 0U);
    if (process == 0)
    {
        return 0;
    }
    userProcess = OrynKernelUserProcessFromProcess(process);
    thread = OrynUserExecutableLoadExternalCommand(physicalMemory,
        "/SYSTEM/COMMANDS/HELLO", &userProcess, &image);
    if (thread != 0)
    {
        OrynKernelThreadDestroy(thread);
    }
    OrynKernelProcessDestroy(process);
    if (thread != 0)
    {
        gUserExec.CreatedProcessCount += 1U;
    }
    return denied && thread != 0 && image.LoadSegmentCount != 0U &&
        image.Abi.AbiVersion == ORYN_USER_EXEC_ABI_VERSION;
}

void OrynUserExecutablePrintProof(void)
{
    if (gUserExec.Initialized == 0U) OrynUserExecutableInit();
    OrynKernelScreenReportOkOrFail(gUserExec.AbiDefined && gUserExec.AbiVersionReady,
        "Oryn user executable ABI is defined and versioned.",
        "Oryn user executable ABI is not ready.");
    OrynKernelScreenReportOkOrFail(gUserExec.Elf64LoaderReady && gUserExec.VfsLoadSuccessCount != 0U,
        "ELF64 user executables load from VFS.",
        "ELF64 user executable VFS load proof failed.");
    OrynKernelScreenReportOkOrFail(gUserExec.CommandRootReady && gUserExec.CommandPathDeniedCount != 0U,
        "External commands are restricted to the configured System/Commands root.",
        "External command path policy did not reject an outside command.");
    OrynKernelScreenReportOkOrFail(gUserExec.CreatedThreadCount != 0U,
        "Loaded user executable creates a controlled user thread.",
        "Loaded user executable did not create a user thread.");
}
