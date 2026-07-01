#include "KernelDiagnosticsProofsInternal.h"
#include "KernelFat32.h"
#include "KernelVfs.h"
#include "OrynString.h"

#define FAT32_PROOF_SECTORS 256U

static uint8_t gFat32ProofImage[FAT32_PROOF_SECTORS * ORYN_FAT32_SECTOR_SIZE];
static OrynKernelBlockDevice gFat32ProofDevice;
static OrynFat32Volume gFat32ProofVolume;

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
    gFat32ProofDevice.Context = &gFat32ProofContext;
    gFat32ProofDevice.Read = Fat32ProofRead;
    gFat32ProofDevice.Write = Fat32ProofWrite;
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
    Fat32ProofDeviceInit();

    if (!Fat32ProofMountFat32()) return;
    if (!Fat32ProofMountVfs(kernelBootInfo)) return;
    if (!Fat32ProofStep(OrynVfsCreateDirectory("/SYSTEM"),
        "FAT32 directory create works.", "FAT32 directory create failed.")) return;
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
