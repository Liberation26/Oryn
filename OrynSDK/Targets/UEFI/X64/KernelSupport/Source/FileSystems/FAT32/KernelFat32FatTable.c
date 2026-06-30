#include "KernelFat32Internal.h"

int OrynFat32GetFatEntry(OrynFat32Volume* volume, uint32_t cluster, uint32_t* value)
{
    uint32_t fat_offset;
    uint32_t sector_lba;
    uint32_t entry_offset;

    if (volume == 0 || value == 0 || cluster >= volume->Info.ClusterCount + 2U)
    {
        return 0;
    }

    fat_offset = cluster * 4U;
    sector_lba = volume->Info.FirstFatSector + (fat_offset / ORYN_FAT32_SECTOR_SIZE);
    entry_offset = fat_offset % ORYN_FAT32_SECTOR_SIZE;
    if (!OrynKernelBlockReadSector(volume->Device, sector_lba, volume->Sector))
    {
        return 0;
    }

    *value = Fat32Read32(volume->Sector, entry_offset) & 0x0FFFFFFFU;
    return 1;
}

int OrynFat32SetFatEntry(OrynFat32Volume* volume, uint32_t cluster, uint32_t value)
{
    uint32_t fat_index;
    uint32_t fat_offset;
    uint32_t sector_lba;
    uint32_t entry_offset;

    if (volume == 0 || cluster >= volume->Info.ClusterCount + 2U)
    {
        return 0;
    }

    fat_offset = cluster * 4U;
    entry_offset = fat_offset % ORYN_FAT32_SECTOR_SIZE;
    for (fat_index = 0; fat_index < volume->Info.FatCount; ++fat_index)
    {
        sector_lba = volume->Info.FirstFatSector +
            (fat_index * volume->Info.FatSizeSectors) +
            (fat_offset / ORYN_FAT32_SECTOR_SIZE);
        if (!OrynKernelBlockReadSector(volume->Device, sector_lba, volume->Sector))
        {
            return 0;
        }

        Fat32Write32(volume->Sector, entry_offset, value & 0x0FFFFFFFU);
        if (!OrynKernelBlockWriteSector(volume->Device, sector_lba, volume->Sector))
        {
            return 0;
        }
    }

    return 1;
}

int Fat32AllocateCluster(OrynFat32Volume* volume, uint32_t* cluster)
{
    uint32_t candidate;
    uint32_t value;
    uint8_t zero[ORYN_FAT32_MAX_CLUSTER_BYTES];

    if (volume == 0 || cluster == 0)
    {
        return 0;
    }

    OrynMemset(zero, 0, sizeof(zero));
    for (candidate = 2U; candidate < volume->Info.ClusterCount + 2U; ++candidate)
    {
        if (!OrynFat32GetFatEntry(volume, candidate, &value))
        {
            return 0;
        }

        if (value == FAT32_FREE)
        {
            if (!OrynFat32SetFatEntry(volume, candidate, FAT32_EOC))
            {
                return 0;
            }

            if (!OrynFat32WriteCluster(volume, candidate, zero))
            {
                return 0;
            }

            *cluster = candidate;
            return 1;
        }
    }

    return 0;
}

int Fat32AppendCluster(OrynFat32Volume* volume, uint32_t chain_start, uint32_t* new_cluster)
{
    uint32_t current = chain_start;
    uint32_t next = 0;

    if (!Fat32AllocateCluster(volume, new_cluster))
    {
        return 0;
    }

    while (1)
    {
        if (!OrynFat32GetFatEntry(volume, current, &next))
        {
            return 0;
        }
        if (Fat32IsEndOfChain(next))
        {
            break;
        }
        current = next;
    }

    return OrynFat32SetFatEntry(volume, current, *new_cluster);
}

int Fat32FreeClusterChain(OrynFat32Volume* volume, uint32_t start_cluster)
{
    uint32_t current = start_cluster;
    uint32_t next;

    while (current >= 2U && current < volume->Info.ClusterCount + 2U)
    {
        if (!OrynFat32GetFatEntry(volume, current, &next))
        {
            return 0;
        }
        if (!OrynFat32SetFatEntry(volume, current, FAT32_FREE))
        {
            return 0;
        }
        if (Fat32IsEndOfChain(next))
        {
            break;
        }
        current = next;
    }

    return 1;
}
