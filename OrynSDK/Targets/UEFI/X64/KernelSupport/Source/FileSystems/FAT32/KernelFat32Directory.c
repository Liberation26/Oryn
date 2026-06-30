#include "KernelFat32Internal.h"

static uint32_t Fat32EntryFirstCluster(const uint8_t* entry)
{
    return ((uint32_t)Fat32Read16(entry, 20U) << 16U) | Fat32Read16(entry, 26U);
}

static void Fat32WriteEntryFirstCluster(uint8_t* entry, uint32_t cluster)
{
    Fat32Write16(entry, 20U, (uint16_t)((cluster >> 16U) & 0xFFFFU));
    Fat32Write16(entry, 26U, (uint16_t)(cluster & 0xFFFFU));
}

static int Fat32EntryMatches(const uint8_t* entry, const char* name)
{
    char entry_name[256];
    if ((entry[11] & ORYN_FAT32_ATTR_LONG_NAME) == ORYN_FAT32_ATTR_LONG_NAME)
    {
        return 0;
    }
    if (!Fat32ReadDirectoryEntryName(entry, entry_name, sizeof(entry_name)))
    {
        return 0;
    }
    return Fat32NamesEqual(entry_name, name);
}

int Fat32FindInDirectory(OrynFat32Volume* volume, uint32_t directory_cluster, const char* name, OrynFat32FileInfo* info)
{
    uint8_t cluster_data[ORYN_FAT32_MAX_CLUSTER_BYTES];
    uint32_t current = directory_cluster;
    uint32_t next;
    uint32_t offset;

    while (current >= 2U)
    {
        if (!OrynFat32ReadCluster(volume, current, cluster_data))
        {
            return 0;
        }

        for (offset = 0U; offset < Fat32ClusterSize(volume); offset += FAT32_DIR_ENTRY_SIZE)
        {
            const uint8_t* entry = cluster_data + offset;
            if (entry[0] == 0U)
            {
                return 0;
            }
            if (Fat32EntryMatches(entry, name))
            {
                if (info != 0)
                {
                    Fat32ReadDirectoryEntryName(entry, info->Name, sizeof(info->Name));
                    info->Attributes = entry[11];
                    info->FirstCluster = Fat32EntryFirstCluster(entry);
                    info->SizeBytes = Fat32Read32(entry, 28U);
                    info->DirectoryCluster = directory_cluster;
                    info->DirectoryOffset = offset;
                }
                return 1;
            }
        }

        if (!OrynFat32GetFatEntry(volume, current, &next) || Fat32IsEndOfChain(next))
        {
            break;
        }
        current = next;
    }

    return 0;
}

int Fat32FindFreeDirectoryEntry(OrynFat32Volume* volume, uint32_t directory_cluster, uint32_t* offset, uint8_t* sector)
{
    uint32_t current = directory_cluster;
    uint32_t next;
    uint32_t entry_offset;

    while (1)
    {
        if (!OrynFat32ReadCluster(volume, current, sector))
        {
            return 0;
        }
        for (entry_offset = 0U; entry_offset < Fat32ClusterSize(volume); entry_offset += FAT32_DIR_ENTRY_SIZE)
        {
            if (sector[entry_offset] == 0U || sector[entry_offset] == 0xE5U)
            {
                *offset = entry_offset;
                return 1;
            }
        }
        if (!OrynFat32GetFatEntry(volume, current, &next))
        {
            return 0;
        }
        if (Fat32IsEndOfChain(next))
        {
            return Fat32AppendCluster(volume, directory_cluster, &current) &&
                OrynFat32ReadCluster(volume, current, sector) && ((*offset = 0U) == 0U || 1);
        }
        current = next;
    }
}

int Fat32WriteDirectoryEntry(OrynFat32Volume* volume, uint32_t directory_cluster, uint32_t offset, const OrynFat32FileInfo* info)
{
    uint8_t cluster_data[ORYN_FAT32_MAX_CLUSTER_BYTES];
    uint8_t short_name[11];
    uint8_t* entry;

    if (!OrynFat32ReadCluster(volume, directory_cluster, cluster_data))
    {
        return 0;
    }

    entry = cluster_data + offset;
    OrynMemset(entry, 0, FAT32_DIR_ENTRY_SIZE);
    Fat32MakeShortName(info->Name, short_name);
    OrynMemcpy(entry, short_name, 11U);
    entry[11] = info->Attributes;
    Fat32WriteEntryFirstCluster(entry, info->FirstCluster);
    Fat32Write32(entry, 28U, info->SizeBytes);
    return OrynFat32WriteCluster(volume, directory_cluster, cluster_data);
}

int OrynFat32ListDirectory(OrynFat32Volume* volume, const char* path, OrynFat32FileInfo* entries, uint32_t max_entries, uint32_t* count)
{
    OrynFat32FileInfo directory;
    uint8_t cluster_data[ORYN_FAT32_MAX_CLUSTER_BYTES];
    uint32_t current;
    uint32_t next;
    uint32_t offset;

    if (count == 0)
    {
        return 0;
    }
    *count = 0;
    current = volume->Info.RootCluster;
    if (path != 0 && path[0] != 0 && !Fat32NamesEqual(path, "/"))
    {
        if (!OrynFat32FindPath(volume, path, &directory) ||
            (directory.Attributes & ORYN_FAT32_ATTR_DIRECTORY) == 0U)
        {
            return 0;
        }
        current = directory.FirstCluster;
    }

    while (current >= 2U)
    {
        if (!OrynFat32ReadCluster(volume, current, cluster_data))
        {
            return 0;
        }
        for (offset = 0U; offset < Fat32ClusterSize(volume); offset += FAT32_DIR_ENTRY_SIZE)
        {
            const uint8_t* entry = cluster_data + offset;
            if (entry[0] == 0U)
            {
                return 1;
            }
            if (entry[0] != 0xE5U && (entry[11] & ORYN_FAT32_ATTR_LONG_NAME) != ORYN_FAT32_ATTR_LONG_NAME)
            {
                if (*count < max_entries && entries != 0)
                {
                    Fat32ReadDirectoryEntryName(entry, entries[*count].Name, sizeof(entries[*count].Name));
                    entries[*count].Attributes = entry[11];
                    entries[*count].FirstCluster = Fat32EntryFirstCluster(entry);
                    entries[*count].SizeBytes = Fat32Read32(entry, 28U);
                }
                *count += 1U;
            }
        }
        if (!OrynFat32GetFatEntry(volume, current, &next) || Fat32IsEndOfChain(next))
        {
            break;
        }
        current = next;
    }
    return 1;
}
