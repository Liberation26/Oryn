#include "KernelFat32Internal.h"

int Fat32SetStatus(OrynFat32Volume* volume, uint32_t status)
{
    if (volume != 0)
    {
        volume->LastStatus = status;
    }
    return 0;
}

uint32_t OrynFat32LastStatus(const OrynFat32Volume* volume)
{
    if (volume == 0)
    {
        return OrynFat32StatusInvalidArgument;
    }
    return volume->LastStatus;
}

int Fat32IsEndOfChain(uint32_t value)
{
    return (value & 0x0FFFFFFFU) >= FAT32_EOC;
}

uint32_t Fat32ClusterSize(const OrynFat32Volume* volume)
{
    return volume->Info.BytesPerSector * volume->Info.SectorsPerCluster;
}

uint32_t Fat32ClusterToSector(const OrynFat32Volume* volume, uint32_t cluster)
{
    return volume->Info.FirstDataSector + ((cluster - 2U) * volume->Info.SectorsPerCluster);
}

int Fat32ClusterIsValid(const OrynFat32Volume* volume, uint32_t cluster)
{
    return volume != 0 && cluster >= 2U && cluster < volume->Info.ClusterCount + 2U;
}

static int Fat32PowerOfTwo(uint32_t value)
{
    return value != 0U && (value & (value - 1U)) == 0U;
}

static int Fat32BootSectorSignatureOk(const uint8_t* sector)
{
    return sector[510] == 0x55U && sector[511] == 0xAAU;
}

static int Fat32ReadFsInfo(OrynFat32Volume* volume)
{
    uint32_t sector;
    volume->Info.ValidFsInfo = 0U;
    volume->Info.FreeClusterHint = 0xFFFFFFFFU;
    volume->Info.NextFreeClusterHint = 2U;
    sector = volume->Info.FsInfoSector;
    if (sector == 0U || sector >= volume->Info.ReservedSectorCount)
    {
        return 1;
    }
    if (!OrynKernelBlockReadSector(volume->Device, sector, volume->Sector))
    {
        return Fat32SetStatus(volume, OrynFat32StatusIoError);
    }
    if (Fat32Read32(volume->Sector, 0U) == FAT32_FSINFO_SIGNATURE1 &&
        Fat32Read32(volume->Sector, 484U) == FAT32_FSINFO_SIGNATURE2 &&
        Fat32Read32(volume->Sector, 508U) == FAT32_FSINFO_SIGNATURE3)
    {
        volume->Info.ValidFsInfo = 1U;
        volume->Info.FreeClusterHint = Fat32Read32(volume->Sector, 488U);
        volume->Info.NextFreeClusterHint = Fat32Read32(volume->Sector, 492U);
        if (!Fat32ClusterIsValid(volume, volume->Info.NextFreeClusterHint))
        {
            volume->Info.NextFreeClusterHint = 2U;
        }
    }
    return 1;
}

int OrynFat32Mount(OrynFat32Volume* volume, OrynKernelBlockDevice* device)
{
    uint32_t total16;
    uint32_t total32;
    uint32_t root_entry_count;
    uint32_t media;

    if (volume == 0 || device == 0 || device->BytesPerSector != ORYN_FAT32_SECTOR_SIZE)
    {
        return 0;
    }

    OrynMemset(volume, 0, sizeof(OrynFat32Volume));
    volume->Device = device;
    if (!OrynKernelBlockReadSector(device, 0U, volume->Sector))
    {
        return Fat32SetStatus(volume, OrynFat32StatusIoError);
    }

    if (!Fat32BootSectorSignatureOk(volume->Sector))
    {
        return Fat32SetStatus(volume, OrynFat32StatusInvalidBootSector);
    }

    volume->Info.BytesPerSector = Fat32Read16(volume->Sector, 11U);
    volume->Info.SectorsPerCluster = volume->Sector[13];
    volume->Info.ReservedSectorCount = Fat32Read16(volume->Sector, 14U);
    volume->Info.FatCount = volume->Sector[16];
    root_entry_count = Fat32Read16(volume->Sector, 17U);
    volume->Info.FatSizeSectors = Fat32Read32(volume->Sector, 36U);
    volume->Info.RootCluster = Fat32Read32(volume->Sector, 44U);
    volume->Info.FsInfoSector = Fat32Read16(volume->Sector, 48U);
    volume->Info.BackupBootSector = Fat32Read16(volume->Sector, 50U);
    media = volume->Sector[21];
    total16 = Fat32Read16(volume->Sector, 19U);
    total32 = Fat32Read32(volume->Sector, 32U);
    volume->Info.TotalSectors = total16 != 0U ? total16 : total32;
    volume->Info.FirstFatSector = volume->Info.ReservedSectorCount;
    volume->Info.FirstDataSector = volume->Info.ReservedSectorCount +
        (volume->Info.FatCount * volume->Info.FatSizeSectors);

    if (volume->Info.BytesPerSector != ORYN_FAT32_SECTOR_SIZE ||
        !Fat32PowerOfTwo(volume->Info.SectorsPerCluster) ||
        volume->Info.SectorsPerCluster > 128U ||
        volume->Info.ReservedSectorCount == 0U ||
        volume->Info.FatCount == 0U || volume->Info.FatCount > 2U ||
        volume->Info.FatSizeSectors == 0U || volume->Info.RootCluster < 2U ||
        root_entry_count != 0U || media == 0U ||
        volume->Info.TotalSectors == 0U ||
        volume->Info.TotalSectors > device->SectorCount ||
        volume->Info.TotalSectors <= volume->Info.FirstDataSector ||
        Fat32ClusterSize(volume) > ORYN_FAT32_MAX_CLUSTER_BYTES)
    {
        return Fat32SetStatus(volume, OrynFat32StatusUnsupportedFormat);
    }

    volume->Info.ClusterCount =
        (volume->Info.TotalSectors - volume->Info.FirstDataSector) / volume->Info.SectorsPerCluster;
    if (volume->Info.ClusterCount < 1U || !Fat32ClusterIsValid(volume, volume->Info.RootCluster))
    {
        return Fat32SetStatus(volume, OrynFat32StatusUnsupportedFormat);
    }

    volume->Mounted = 1U;
    volume->LastStatus = OrynFat32StatusOk;
    if (!Fat32ReadFsInfo(volume))
    {
        return 0;
    }
    return 1;
}

int OrynFat32Flush(OrynFat32Volume* volume)
{
    if (volume == 0 || volume->Mounted == 0U)
    {
        return 0;
    }
    return Fat32UpdateFsInfo(volume);
}
