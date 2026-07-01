#include "KernelFat32Internal.h"

int OrynFat32GetFatEntry(OrynFat32Volume* volume, uint32_t cluster, uint32_t* value)
{
    uint32_t fat_offset;
    uint32_t sector_lba;
    uint32_t entry_offset;

    if (volume == 0 || value == 0 || !Fat32ClusterIsValid(volume, cluster))
    {
        return Fat32SetStatus(volume, OrynFat32StatusInvalidCluster);
    }

    fat_offset = cluster * 4U;
    sector_lba = volume->Info.FirstFatSector + (fat_offset / ORYN_FAT32_SECTOR_SIZE);
    entry_offset = fat_offset % ORYN_FAT32_SECTOR_SIZE;
    if (!OrynKernelBlockReadSector(volume->Device, sector_lba, volume->Sector))
    {
        return Fat32SetStatus(volume, OrynFat32StatusIoError);
    }

    *value = Fat32Read32(volume->Sector, entry_offset) & 0x0FFFFFFFU;
    volume->LastStatus = OrynFat32StatusOk;
    return 1;
}

int OrynFat32SetFatEntry(OrynFat32Volume* volume, uint32_t cluster, uint32_t value)
{
    uint32_t fat_index;
    uint32_t fat_offset;
    uint32_t sector_lba;
    uint32_t entry_offset;
    uint32_t old_high;

    if (volume == 0 || !Fat32ClusterIsValid(volume, cluster))
    {
        return Fat32SetStatus(volume, OrynFat32StatusInvalidCluster);
    }
    if (volume->ReadOnly != 0U)
    {
        return Fat32SetStatus(volume, OrynFat32StatusReadOnly);
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
            return Fat32SetStatus(volume, OrynFat32StatusIoError);
        }

        old_high = Fat32Read32(volume->Sector, entry_offset) & 0xF0000000U;
        Fat32Write32(volume->Sector, entry_offset, old_high | (value & 0x0FFFFFFFU));
        if (!OrynKernelBlockWriteSector(volume->Device, sector_lba, volume->Sector))
        {
            return Fat32SetStatus(volume, OrynFat32StatusIoError);
        }
    }

    volume->LastStatus = OrynFat32StatusOk;
    return 1;
}

int Fat32UpdateFsInfo(OrynFat32Volume* volume)
{
    if (volume == 0 || volume->Info.ValidFsInfo == 0U)
    {
        return 1;
    }
    if (volume->ReadOnly != 0U)
    {
        return Fat32SetStatus(volume, OrynFat32StatusReadOnly);
    }
    if (!OrynKernelBlockReadSector(volume->Device, volume->Info.FsInfoSector, volume->Sector))
    {
        return Fat32SetStatus(volume, OrynFat32StatusIoError);
    }
    Fat32Write32(volume->Sector, 488U, volume->Info.FreeClusterHint);
    Fat32Write32(volume->Sector, 492U, volume->Info.NextFreeClusterHint);
    if (!OrynKernelBlockWriteSector(volume->Device, volume->Info.FsInfoSector, volume->Sector))
    {
        return Fat32SetStatus(volume, OrynFat32StatusIoError);
    }
    return 1;
}

int OrynFat32GetFreeClusterEstimate(OrynFat32Volume* volume, uint32_t* free_clusters)
{
    uint32_t candidate;
    uint32_t value;
    uint32_t count = 0U;
    if (volume == 0 || free_clusters == 0)
    {
        return 0;
    }
    if (volume->Info.ValidFsInfo != 0U && volume->Info.FreeClusterHint != 0xFFFFFFFFU)
    {
        *free_clusters = volume->Info.FreeClusterHint;
        return 1;
    }
    for (candidate = 2U; candidate < volume->Info.ClusterCount + 2U; ++candidate)
    {
        if (!OrynFat32GetFatEntry(volume, candidate, &value))
        {
            return 0;
        }
        if (value == FAT32_FREE)
        {
            ++count;
        }
    }
    volume->Info.ValidFsInfo = 1U;
    volume->Info.FreeClusterHint = count;
    volume->Info.NextFreeClusterHint = 2U;
    *free_clusters = count;
    return 1;
}

static uint32_t Fat32NextAllocationStart(OrynFat32Volume* volume)
{
    if (volume->Info.ValidFsInfo != 0U && Fat32ClusterIsValid(volume, volume->Info.NextFreeClusterHint))
    {
        return volume->Info.NextFreeClusterHint;
    }
    return 2U;
}

int Fat32AllocateCluster(OrynFat32Volume* volume, uint32_t* cluster)
{
    uint32_t candidate;
    uint32_t pass;
    uint32_t value;
    uint8_t zero[ORYN_FAT32_MAX_CLUSTER_BYTES];
    uint32_t start;
    uint32_t end;

    if (volume == 0 || cluster == 0)
    {
        return 0;
    }

    OrynMemset(zero, 0, sizeof(zero));
    start = Fat32NextAllocationStart(volume);
    end = volume->Info.ClusterCount + 2U;
    for (pass = 0U; pass < 2U; ++pass)
    {
        uint32_t first = pass == 0U ? start : 2U;
        uint32_t last = pass == 0U ? end : start;
        for (candidate = first; candidate < last; ++candidate)
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
                if (volume->Info.ValidFsInfo != 0U && volume->Info.FreeClusterHint != 0xFFFFFFFFU &&
                    volume->Info.FreeClusterHint > 0U)
                {
                    volume->Info.FreeClusterHint -= 1U;
                }
                volume->Info.NextFreeClusterHint = candidate + 1U;
                if (!Fat32ClusterIsValid(volume, volume->Info.NextFreeClusterHint))
                {
                    volume->Info.NextFreeClusterHint = 2U;
                }
                *cluster = candidate;
                return 1;
            }
        }
    }

    return Fat32SetStatus(volume, OrynFat32StatusNoSpace);
}

int Fat32AppendCluster(OrynFat32Volume* volume, uint32_t chain_start, uint32_t* new_cluster)
{
    uint32_t current = chain_start;
    uint32_t next = 0;

    if (!Fat32ClusterIsValid(volume, chain_start) || !Fat32AllocateCluster(volume, new_cluster))
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
        if (!Fat32ClusterIsValid(volume, next))
        {
            return Fat32SetStatus(volume, OrynFat32StatusInvalidCluster);
        }
        current = next;
    }

    return OrynFat32SetFatEntry(volume, current, *new_cluster);
}

int Fat32FreeClusterChain(OrynFat32Volume* volume, uint32_t start_cluster)
{
    uint32_t current = start_cluster;
    uint32_t next;

    while (Fat32ClusterIsValid(volume, current))
    {
        if (!OrynFat32GetFatEntry(volume, current, &next))
        {
            return 0;
        }
        if (!OrynFat32SetFatEntry(volume, current, FAT32_FREE))
        {
            return 0;
        }
        if (volume->Info.ValidFsInfo != 0U && volume->Info.FreeClusterHint != 0xFFFFFFFFU)
        {
            volume->Info.FreeClusterHint += 1U;
        }
        if (Fat32IsEndOfChain(next))
        {
            break;
        }
        current = next;
    }

    return 1;
}

int Fat32TruncateChain(OrynFat32Volume* volume, uint32_t start_cluster, uint32_t keep_count)
{
    uint32_t current = start_cluster;
    uint32_t next = FAT32_EOC;
    uint32_t index;

    if (keep_count == 0U)
    {
        return Fat32FreeClusterChain(volume, start_cluster);
    }
    if (!Fat32ClusterIsValid(volume, start_cluster))
    {
        return 0;
    }
    for (index = 1U; index < keep_count; ++index)
    {
        if (!OrynFat32GetFatEntry(volume, current, &next) || Fat32IsEndOfChain(next))
        {
            return 1;
        }
        if (!Fat32ClusterIsValid(volume, next))
        {
            return Fat32SetStatus(volume, OrynFat32StatusInvalidCluster);
        }
        current = next;
    }
    if (!OrynFat32GetFatEntry(volume, current, &next))
    {
        return 0;
    }
    if (Fat32IsEndOfChain(next))
    {
        return 1;
    }
    if (!OrynFat32SetFatEntry(volume, current, FAT32_EOC))
    {
        return 0;
    }
    return Fat32FreeClusterChain(volume, next);
}

int OrynFat32CheckVolume(OrynFat32Volume* volume)
{
    uint32_t cluster;
    uint32_t value;
    if (volume == 0 || volume->Mounted == 0U)
    {
        return 0;
    }
    for (cluster = 2U; cluster < volume->Info.ClusterCount + 2U; ++cluster)
    {
        if (!OrynFat32GetFatEntry(volume, cluster, &value))
        {
            return 0;
        }
        if (value != FAT32_FREE && value != FAT32_BAD && !Fat32IsEndOfChain(value) &&
            !Fat32ClusterIsValid(volume, value))
        {
            return Fat32SetStatus(volume, OrynFat32StatusInvalidCluster);
        }
    }
    volume->LastStatus = OrynFat32StatusOk;
    return 1;
}
