#include "KernelFat32Internal.h"

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

static int Fat32BootSectorSignatureOk(const uint8_t* sector)
{
    return sector[510] == 0x55U && sector[511] == 0xAAU;
}

int OrynFat32Mount(OrynFat32Volume* volume, OrynKernelBlockDevice* device)
{
    uint32_t total16;
    uint32_t total32;

    if (volume == 0 || device == 0 || device->BytesPerSector != ORYN_FAT32_SECTOR_SIZE)
    {
        return 0;
    }

    OrynMemset(volume, 0, sizeof(OrynFat32Volume));
    volume->Device = device;
    if (!OrynKernelBlockReadSector(device, 0U, volume->Sector))
    {
        return 0;
    }

    if (!Fat32BootSectorSignatureOk(volume->Sector))
    {
        return 0;
    }

    volume->Info.BytesPerSector = Fat32Read16(volume->Sector, 11U);
    volume->Info.SectorsPerCluster = volume->Sector[13];
    volume->Info.ReservedSectorCount = Fat32Read16(volume->Sector, 14U);
    volume->Info.FatCount = volume->Sector[16];
    volume->Info.FatSizeSectors = Fat32Read32(volume->Sector, 36U);
    volume->Info.RootCluster = Fat32Read32(volume->Sector, 44U);
    total16 = Fat32Read16(volume->Sector, 19U);
    total32 = Fat32Read32(volume->Sector, 32U);
    volume->Info.TotalSectors = total16 != 0U ? total16 : total32;
    volume->Info.FirstFatSector = volume->Info.ReservedSectorCount;
    volume->Info.FirstDataSector = volume->Info.ReservedSectorCount +
        (volume->Info.FatCount * volume->Info.FatSizeSectors);

    if (volume->Info.BytesPerSector != ORYN_FAT32_SECTOR_SIZE ||
        volume->Info.SectorsPerCluster == 0U || volume->Info.FatCount == 0U ||
        volume->Info.FatSizeSectors == 0U || volume->Info.RootCluster < 2U ||
        volume->Info.TotalSectors <= volume->Info.FirstDataSector ||
        Fat32ClusterSize(volume) > ORYN_FAT32_MAX_CLUSTER_BYTES)
    {
        return 0;
    }

    volume->Info.ClusterCount =
        (volume->Info.TotalSectors - volume->Info.FirstDataSector) / volume->Info.SectorsPerCluster;
    return volume->Info.ClusterCount >= 1U;
}
