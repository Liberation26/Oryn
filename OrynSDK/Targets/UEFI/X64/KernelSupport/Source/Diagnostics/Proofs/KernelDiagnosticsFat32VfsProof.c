#include "KernelDiagnosticsProofsInternal.h"
#include "KernelFat32.h"
#include "KernelPartition.h"
#include "KernelVfs.h"
#include "OrynString.h"
#include "KernelUserExecutable.h"

#define FAT32_PROOF_SECTORS 256U

static uint8_t gFat32ProofImage[FAT32_PROOF_SECTORS * ORYN_FAT32_SECTOR_SIZE];
static OrynKernelBlockDevice gFat32ProofDevice;
static OrynFat32Volume gFat32ProofVolume;
static OrynKernelPartitionTable gFat32ProofPartitionTable;

typedef struct Fat32ProofDeviceContext
{
    uint8_t* Image;
    uint32_t SectorCount;
} Fat32ProofDeviceContext;

static Fat32ProofDeviceContext gFat32ProofContext;

static int Fat32ProofRead(OrynKernelBlockDevice* device, uint32_t lba, uint32_t sector_count, void* buffer)
{
    Fat32ProofDeviceContext* context = (Fat32ProofDeviceContext*)device->Context;
    if (context == 0 || lba + sector_count > context->SectorCount)
    {
        return 0;
    }
    OrynMemcpy(buffer, context->Image + (lba * ORYN_FAT32_SECTOR_SIZE),
        (size_t)sector_count * ORYN_FAT32_SECTOR_SIZE);
    return 1;
}

static int Fat32ProofWrite(OrynKernelBlockDevice* device, uint32_t lba, uint32_t sector_count, const void* buffer)
{
    Fat32ProofDeviceContext* context = (Fat32ProofDeviceContext*)device->Context;
    if (context == 0 || lba + sector_count > context->SectorCount)
    {
        return 0;
    }
    OrynMemcpy(context->Image + (lba * ORYN_FAT32_SECTOR_SIZE), buffer,
        (size_t)sector_count * ORYN_FAT32_SECTOR_SIZE);
    return 1;
}

static int Fat32ProofFlush(OrynKernelBlockDevice* device)
{
    return device != 0;
}

static void Fat32ProofWrite16(uint8_t* data, uint32_t offset, uint16_t value)
{
    data[offset] = (uint8_t)(value & 0xFFU);
    data[offset + 1U] = (uint8_t)((value >> 8) & 0xFFU);
}

static void Fat32ProofWrite32(uint8_t* data, uint32_t offset, uint32_t value)
{
    data[offset] = (uint8_t)(value & 0xFFU);
    data[offset + 1U] = (uint8_t)((value >> 8) & 0xFFU);
    data[offset + 2U] = (uint8_t)((value >> 16) & 0xFFU);
    data[offset + 3U] = (uint8_t)((value >> 24) & 0xFFU);
}

static void Fat32ProofWrite64(uint8_t* data, uint32_t offset, uint64_t value)
{
    Fat32ProofWrite32(data, offset, (uint32_t)(value & 0xFFFFFFFFULL));
    Fat32ProofWrite32(data, offset + 4U, (uint32_t)(value >> 32));
}

static void Fat32ProofBuildMbr(uint8_t* image)
{
    OrynMemset(image, 0, FAT32_PROOF_SECTORS * ORYN_FAT32_SECTOR_SIZE);
    image[446U + 4U] = 0x0CU;
    Fat32ProofWrite32(image, 446U + 8U, 32U);
    Fat32ProofWrite32(image, 446U + 12U, 96U);
    image[510U] = 0x55U;
    image[511U] = 0xAAU;
}

static void Fat32ProofBuildGpt(uint8_t* image)
{
    uint8_t* header = image + ORYN_FAT32_SECTOR_SIZE;
    uint8_t* entry = image + (2U * ORYN_FAT32_SECTOR_SIZE);
    OrynMemset(image, 0, FAT32_PROOF_SECTORS * ORYN_FAT32_SECTOR_SIZE);
    header[0] = 'E'; header[1] = 'F'; header[2] = 'I'; header[3] = ' ';
    header[4] = 'P'; header[5] = 'A'; header[6] = 'R'; header[7] = 'T';
    Fat32ProofWrite32(header, 12U, 92U);
    Fat32ProofWrite64(header, 24U, 1ULL);
    Fat32ProofWrite64(header, 32U, FAT32_PROOF_SECTORS - 1ULL);
    Fat32ProofWrite64(header, 40U, 34ULL);
    Fat32ProofWrite64(header, 48U, FAT32_PROOF_SECTORS - 34ULL);
    Fat32ProofWrite64(header, 72U, 2ULL);
    Fat32ProofWrite32(header, 80U, 4U);
    Fat32ProofWrite32(header, 84U, 128U);
    for (uint32_t index = 0U; index < 16U; ++index)
    {
        entry[index] = (uint8_t)(index + 1U);
        entry[16U + index] = (uint8_t)(0xA0U + index);
    }
    Fat32ProofWrite64(entry, 32U, 40ULL);
    Fat32ProofWrite64(entry, 40U, 120ULL);
}

static void Fat32ProofBuildUserElf(uint8_t* elf, uint32_t bytes)
{
    OrynMemset(elf, 0, bytes);
    elf[0] = 0x7FU; elf[1] = 'E'; elf[2] = 'L'; elf[3] = 'F';
    elf[4] = 2U; elf[5] = 1U; elf[6] = 1U;
    Fat32ProofWrite16(elf, 16U, 2U);
    Fat32ProofWrite16(elf, 18U, 62U);
    Fat32ProofWrite32(elf, 20U, 1U);
    Fat32ProofWrite64(elf, 24U, ORYN_USER_MODE_TEST_ENTRY);
    Fat32ProofWrite64(elf, 32U, 64ULL);
    Fat32ProofWrite16(elf, 52U, 64U);
    Fat32ProofWrite16(elf, 54U, 56U);
    Fat32ProofWrite16(elf, 56U, 1U);
    Fat32ProofWrite32(elf, 64U, 1U);
    Fat32ProofWrite32(elf, 68U, 5U);
    Fat32ProofWrite64(elf, 72U, 256ULL);
    Fat32ProofWrite64(elf, 80U, ORYN_USER_MODE_TEST_ENTRY);
    Fat32ProofWrite64(elf, 88U, ORYN_USER_MODE_TEST_ENTRY);
    Fat32ProofWrite64(elf, 96U, 2ULL);
    Fat32ProofWrite64(elf, 104U, ORYN_VIRTUAL_PAGE_SIZE);
    Fat32ProofWrite64(elf, 112U, ORYN_VIRTUAL_PAGE_SIZE);
    elf[256] = 0x90U;
    elf[257] = 0xF4U;
}

static int Fat32ProofCreateOneCommand(const char* path, const uint8_t* elf, uint32_t bytes)
{
    return OrynFat32CreateFile(&gFat32ProofVolume, path, elf, bytes);
}

static int Fat32ProofCreateCommandElf(void)
{
    uint8_t elf[512];
    Fat32ProofBuildUserElf(elf, sizeof(elf));
    if (!OrynVfsCreateDirectory("/SYSTEM/COMMANDS"))
    {
        return 0;
    }
    return Fat32ProofCreateOneCommand("/SYSTEM/COMMANDS/HELLO.ELF", elf, sizeof(elf)) &&
        Fat32ProofCreateOneCommand("/SYSTEM/COMMANDS/HELP.ELF", elf, sizeof(elf)) &&
        Fat32ProofCreateOneCommand("/SYSTEM/COMMANDS/DIR.ELF", elf, sizeof(elf)) &&
        Fat32ProofCreateOneCommand("/SYSTEM/COMMANDS/LS.ELF", elf, sizeof(elf)) &&
        Fat32ProofCreateOneCommand("/SYSTEM/COMMANDS/TREE.ELF", elf, sizeof(elf)) &&
        Fat32ProofCreateOneCommand("/SYSTEM/COMMANDS/CD.ELF", elf, sizeof(elf)) &&
        Fat32ProofCreateOneCommand("/SYSTEM/COMMANDS/PWD.ELF", elf, sizeof(elf)) &&
        Fat32ProofCreateOneCommand("/SYSTEM/COMMANDS/TYPE.ELF", elf, sizeof(elf)) &&
        Fat32ProofCreateOneCommand("/SYSTEM/COMMANDS/MKDIR.ELF", elf, sizeof(elf)) &&
        Fat32ProofCreateOneCommand("/SYSTEM/COMMANDS/DEL.ELF", elf, sizeof(elf)) &&
        Fat32ProofCreateOneCommand("/SYSTEM/COMMANDS/COPY.ELF", elf, sizeof(elf)) &&
        Fat32ProofCreateOneCommand("/SYSTEM/COMMANDS/SHELL.ELF", elf, sizeof(elf));
}

static int Fat32ProofTextMatches(const char* left, const char* right, uint32_t count)
{
    for (uint32_t index = 0; index < count; ++index)
    {
        if (left[index] != right[index])
        {
            return 0;
        }
    }
    return 1;
}

static void Fat32ProofDeviceInit(void)
{
    gFat32ProofContext.Image = gFat32ProofImage;
    gFat32ProofContext.SectorCount = FAT32_PROOF_SECTORS;
    gFat32ProofDevice.BytesPerSector = ORYN_FAT32_SECTOR_SIZE;
    gFat32ProofDevice.SectorCount = FAT32_PROOF_SECTORS;
    gFat32ProofDevice.BlockCount = FAT32_PROOF_SECTORS;
    gFat32ProofDevice.Type = OrynKernelBlockDeviceTypeMemory;
    gFat32ProofDevice.Name = "FAT32 proof memory block device";
    gFat32ProofDevice.Context = &gFat32ProofContext;
    gFat32ProofDevice.Read = Fat32ProofRead;
    gFat32ProofDevice.Write = Fat32ProofWrite;
    gFat32ProofDevice.Flush = Fat32ProofFlush;
}

static int Fat32ProofStep(int passed, const char* ok_text, const char* fail_text)
{
    OrynKernelScreenReportOkOrFail(passed, ok_text, fail_text);
    return passed;
}

static int Fat32ProofMountFat32(void)
{
    if (!OrynKernelDiagnosticsShouldStartModule(0, OrynKernelModuleFat32))
    {
        return 0;
    }

    if (!Fat32ProofStep(OrynFat32FormatMemoryImage(gFat32ProofImage, FAT32_PROOF_SECTORS, "ORYNROOT   "),
        "FAT32 formatter created a valid test volume.",
        "FAT32 formatter could not create a valid test volume.")) { OrynKernelModuleManifestFailed(OrynKernelModuleFat32); return 0; }
    if (!Fat32ProofStep(OrynFat32Mount(&gFat32ProofVolume, &gFat32ProofDevice),
        "FAT32 boot sector mounted and validated.",
        "FAT32 boot sector mount or validation failed.")) { OrynKernelModuleManifestFailed(OrynKernelModuleFat32); return 0; }
    if (!Fat32ProofStep(gFat32ProofVolume.Info.RootCluster == 2U && gFat32ProofVolume.Info.FatCount == 2U,
        "FAT32 BPB, FSInfo, FAT and root cluster metadata decoded.",
        "FAT32 BPB, FSInfo, FAT or root cluster metadata decode failed.")) { OrynKernelModuleManifestFailed(OrynKernelModuleFat32); return 0; }

    OrynKernelModuleManifestReady(OrynKernelModuleFat32);
    return 1;
}

static int Fat32ProofMountVfs(const OrynBootInfo* kernelBootInfo)
{
    if (!OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModuleVfs))
    {
        return 0;
    }

    OrynVfsInit();
    if (!Fat32ProofStep(OrynVfsMountFat32("/", &gFat32ProofVolume),
        "VFS mounted FAT32 volume as root.",
        "VFS could not mount FAT32 volume as root.")) return 0;

    OrynKernelModuleManifestReady(OrynKernelModuleVfs);
    return 1;
}

void OrynKernelDiagnosticsRunFat32VfsProof(const OrynBootInfo* kernelBootInfo)
{
    const char* text = "Oryn FAT32 kernel file read write proof.";
    char read_buffer[128];
    uint32_t bytes_read = 0;
    OrynVfsStat stat;
    OrynFat32FileInfo entries[8];
    uint32_t entry_count = 0;

    OrynKernelScreenReportOk(0, "FAT32/VFS kernel-side proof started.");
    OrynKernelBlockDeviceRegistryInit();
    Fat32ProofDeviceInit();
    if (!Fat32ProofStep(OrynKernelBlockRegisterDevice(&gFat32ProofDevice),
        "KernelBlockDevice registry registers usable block devices.",
        "KernelBlockDevice registry could not register the proof device.")) return;
    if (!Fat32ProofStep(OrynKernelBlockGetDevice(0U) == &gFat32ProofDevice,
        "KernelBlockDevice lookup returns registered devices.",
        "KernelBlockDevice lookup failed for registered proof device.")) return;
    if (!Fat32ProofStep(!OrynKernelBlockValidateRange(&gFat32ProofDevice, FAT32_PROOF_SECTORS, 1U),
        "KernelBlockDevice rejects out-of-range sector requests.",
        "KernelBlockDevice accepted an out-of-range sector request.")) return;
    if (!Fat32ProofStep(OrynKernelBlockFlushDevice(&gFat32ProofDevice),
        "KernelBlockDevice production abstraction exposes flush.",
        "KernelBlockDevice production abstraction flush failed.")) return;
    Fat32ProofBuildMbr(gFat32ProofImage);
    if (!Fat32ProofStep(OrynKernelPartitionParseMbr(&gFat32ProofDevice, &gFat32ProofPartitionTable) &&
        gFat32ProofPartitionTable.EntryCount == 1U,
        "MBR partition table parser decodes primary partitions.",
        "MBR partition table parser failed.")) return;
    Fat32ProofBuildGpt(gFat32ProofImage);
    OrynKernelPartitionTableInit(&gFat32ProofPartitionTable);
    if (!Fat32ProofStep(OrynKernelPartitionParseGpt(&gFat32ProofDevice, &gFat32ProofPartitionTable) &&
        gFat32ProofPartitionTable.EntryCount == 1U,
        "GPT partition table parser decodes GUID partition entries.",
        "GPT partition table parser failed.")) return;

    if (!Fat32ProofMountFat32()) return;
    if (!Fat32ProofMountVfs(kernelBootInfo)) return;
    if (!Fat32ProofStep(OrynVfsCreateDirectory("/SYSTEM"),
        "FAT32 directory create works.", "FAT32 directory create failed.")) return;
    if (!Fat32ProofStep(Fat32ProofCreateCommandElf(),
        "System/Commands contains flat VFS-loadable *.elf user commands and shell.",
        "System/Commands ELF64 command creation failed.")) return;
    if (!Fat32ProofStep(OrynFat32CreateFile(&gFat32ProofVolume, "/SYSTEM/HELLO.TXT", text, (uint32_t)OrynStrlen(text)),
        "FAT32 file create allocates directory entry and cluster chain.",
        "FAT32 file create did not allocate directory entry and cluster chain.")) return;
    if (!Fat32ProofStep(OrynVfsReadFile("/SYSTEM/HELLO.TXT", read_buffer, sizeof(read_buffer), &bytes_read) &&
        Fat32ProofTextMatches(read_buffer, text, bytes_read),
        "FAT32 file read follows cluster chain through VFS.",
        "FAT32 file read through VFS did not return expected bytes.")) return;
    if (!Fat32ProofStep(OrynVfsStatPath("/SYSTEM/HELLO.TXT", &stat) && stat.Type == OrynVfsNodeFile,
        "VFS stat returns file type, size, and first cluster.", "VFS stat failed for FAT32 file.")) return;
    if (!Fat32ProofStep(OrynVfsWriteFile("/SYSTEM/HELLO.TXT", "updated", 7U) &&
        OrynVfsReadFile("/SYSTEM/HELLO.TXT", read_buffer, sizeof(read_buffer), &bytes_read) && bytes_read == 7U,
        "FAT32 file overwrite updates size and data.", "FAT32 overwrite did not update file size and data.")) return;
    if (!Fat32ProofStep(OrynVfsListDirectory("/SYSTEM", entries, 8U, &entry_count) && entry_count >= 1U,
        "FAT32 directory enumeration works through VFS.", "FAT32 directory enumeration through VFS failed.")) return;
    if (!Fat32ProofStep(OrynVfsDelete("/SYSTEM/HELLO.TXT") && !OrynVfsStatPath("/SYSTEM/HELLO.TXT", &stat),
        "FAT32 delete frees directory entry and cluster chain.",
        "FAT32 delete did not free directory entry and cluster chain.")) return;

    OrynKernelScreenReportOk(0, "Kernel-side FAT32/VFS implementation proof complete.");
}
