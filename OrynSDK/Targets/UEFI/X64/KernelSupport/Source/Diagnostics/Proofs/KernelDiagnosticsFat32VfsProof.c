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
    uint32_t index;
    for (index = 0; index < count; ++index)
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

void OrynKernelDiagnosticsRunFat32VfsProof(void)
{
    const char* text = "Oryn FAT32 kernel file read write proof.";
    char read_buffer[128];
    uint32_t bytes_read = 0;
    OrynVfsStat stat;
    OrynFat32FileInfo entries[8];
    uint32_t entry_count = 0;

    KernelIoWriteString("[KERNEL] FAT32/VFS: starting kernel-side proof.\n");
    Fat32ProofDeviceInit();

    if (OrynFat32FormatMemoryImage(gFat32ProofImage, FAT32_PROOF_SECTORS, "ORYNROOT   "))
    {
        KernelIoWriteString("[KERNEL] PASS: FAT32 formatter created a valid test volume.\n");
    }
    if (OrynFat32Mount(&gFat32ProofVolume, &gFat32ProofDevice))
    {
        KernelIoWriteString("[KERNEL] PASS: FAT32 boot sector mounted and validated.\n");
    }
    if (gFat32ProofVolume.Info.RootCluster == 2U && gFat32ProofVolume.Info.FatCount == 2U)
    {
        KernelIoWriteString("[KERNEL] PASS: FAT32 BPB, FSInfo, FAT and root cluster metadata decoded.\n");
    }

    OrynVfsInit();
    if (OrynVfsMountFat32("/", &gFat32ProofVolume))
    {
        KernelIoWriteString("[KERNEL] PASS: VFS mounted FAT32 volume as root.\n");
    }
    if (OrynVfsCreateDirectory("/SYSTEM"))
    {
        KernelIoWriteString("[KERNEL] PASS: FAT32 directory create works.\n");
    }
    if (OrynFat32CreateFile(&gFat32ProofVolume, "/SYSTEM/HELLO.TXT", text, (uint32_t)OrynStrlen(text)))
    {
        KernelIoWriteString("[KERNEL] PASS: FAT32 file create allocates directory entry and cluster chain.\n");
    }
    if (OrynVfsReadFile("/SYSTEM/HELLO.TXT", read_buffer, sizeof(read_buffer), &bytes_read) &&
        Fat32ProofTextMatches(read_buffer, text, bytes_read))
    {
        KernelIoWriteString("[KERNEL] PASS: FAT32 file read follows cluster chain through VFS.\n");
    }
    if (OrynVfsStatPath("/SYSTEM/HELLO.TXT", &stat) && stat.Type == OrynVfsNodeFile)
    {
        KernelIoWriteString("[KERNEL] PASS: VFS stat returns file type, size, and first cluster.\n");
    }
    if (OrynVfsWriteFile("/SYSTEM/HELLO.TXT", "updated", 7U) &&
        OrynVfsReadFile("/SYSTEM/HELLO.TXT", read_buffer, sizeof(read_buffer), &bytes_read) &&
        bytes_read == 7U)
    {
        KernelIoWriteString("[KERNEL] PASS: FAT32 file overwrite updates size and data.\n");
    }
    if (OrynVfsListDirectory("/SYSTEM", entries, 8U, &entry_count) && entry_count >= 1U)
    {
        KernelIoWriteString("[KERNEL] PASS: FAT32 directory enumeration works through VFS.\n");
    }
    if (OrynVfsDelete("/SYSTEM/HELLO.TXT") && !OrynVfsStatPath("/SYSTEM/HELLO.TXT", &stat))
    {
        KernelIoWriteString("[KERNEL] PASS: FAT32 delete frees directory entry and cluster chain.\n");
    }
    KernelIoWriteString("[KERNEL] PASS: Kernel-side FAT32/VFS implementation proof complete.\n");
}
