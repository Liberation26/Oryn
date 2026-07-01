#include "KernelFat32Internal.h"

int OrynFat32ReadCluster(OrynFat32Volume* volume, uint32_t cluster, void* buffer)
{
    uint32_t first_sector;
    uint32_t sector_index;
    uint8_t* output = (uint8_t*)buffer;

    if (volume == 0 || buffer == 0 || !Fat32ClusterIsValid(volume, cluster))
    {
        return Fat32SetStatus(volume, OrynFat32StatusInvalidCluster);
    }

    first_sector = Fat32ClusterToSector(volume, cluster);
    for (sector_index = 0; sector_index < volume->Info.SectorsPerCluster; ++sector_index)
    {
        if (!OrynKernelBlockReadSector(volume->Device, first_sector + sector_index,
            output + (sector_index * ORYN_FAT32_SECTOR_SIZE)))
        {
            return Fat32SetStatus(volume, OrynFat32StatusIoError);
        }
    }

    volume->LastStatus = OrynFat32StatusOk;
    return 1;
}

int OrynFat32WriteCluster(OrynFat32Volume* volume, uint32_t cluster, const void* buffer)
{
    uint32_t first_sector;
    uint32_t sector_index;
    const uint8_t* input = (const uint8_t*)buffer;

    if (volume == 0 || buffer == 0 || !Fat32ClusterIsValid(volume, cluster))
    {
        return Fat32SetStatus(volume, OrynFat32StatusInvalidCluster);
    }
    if (volume->ReadOnly != 0U)
    {
        return Fat32SetStatus(volume, OrynFat32StatusReadOnly);
    }

    first_sector = Fat32ClusterToSector(volume, cluster);
    for (sector_index = 0; sector_index < volume->Info.SectorsPerCluster; ++sector_index)
    {
        if (!OrynKernelBlockWriteSector(volume->Device, first_sector + sector_index,
            input + (sector_index * ORYN_FAT32_SECTOR_SIZE)))
        {
            return Fat32SetStatus(volume, OrynFat32StatusIoError);
        }
    }

    volume->LastStatus = OrynFat32StatusOk;
    return 1;
}
