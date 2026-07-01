#include "KernelFat32Internal.h"

int Fat32DirectoryIsEmpty(OrynFat32Volume* volume, uint32_t directory_cluster)
{
    uint8_t cluster_data[ORYN_FAT32_MAX_CLUSTER_BYTES];
    uint32_t current = directory_cluster;
    uint32_t next;
    uint32_t offset;
    while (Fat32ClusterIsValid(volume, current))
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
                char name[16];
                if (Fat32ReadDirectoryEntryName(entry, name, sizeof(name)) &&
                    !Fat32NamesEqual(name, ".") && !Fat32NamesEqual(name, ".."))
                {
                    return 0;
                }
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

int OrynFat32CreateDirectory(OrynFat32Volume* volume, const char* path)
{
    uint32_t parent;
    uint32_t offset;
    uint32_t cluster;
    char leaf[256];
    uint8_t sector[ORYN_FAT32_SECTOR_SIZE];
    OrynFat32FileInfo info;

    if (volume == 0 || path == 0 || !Fat32PathIsSafeShortName(path) || Fat32NamesEqual(path, "/"))
    {
        return Fat32SetStatus(volume, OrynFat32StatusPathInvalid);
    }
    if (!Fat32ResolveParent(volume, path, &parent, leaf, sizeof(leaf)))
    {
        return Fat32SetStatus(volume, OrynFat32StatusNotFound);
    }
    if (Fat32FindInDirectory(volume, parent, leaf, 0))
    {
        return Fat32SetStatus(volume, OrynFat32StatusAlreadyExists);
    }
    if (!Fat32FindFreeDirectoryEntry(volume, parent, &offset, sector))
    {
        return 0;
    }
    if (!Fat32AllocateCluster(volume, &cluster))
    {
        return 0;
    }

    OrynMemset(&info, 0, sizeof(info));
    OrynMemcpy(info.Name, leaf, OrynStrlen(leaf) + 1U);
    info.Attributes = ORYN_FAT32_ATTR_DIRECTORY;
    info.FirstCluster = cluster;
    info.DirectoryCluster = parent;
    info.DirectoryOffset = offset;
    if (!Fat32WriteDirectoryEntry(volume, parent, offset, &info))
    {
        (void)Fat32FreeClusterChain(volume, cluster);
        return 0;
    }
    return 1;
}

int OrynFat32Delete(OrynFat32Volume* volume, const char* path)
{
    OrynFat32FileInfo info;
    uint8_t cluster_data[ORYN_FAT32_MAX_CLUSTER_BYTES];

    if (volume == 0 || path == 0 || !Fat32PathIsSafeShortName(path) || Fat32NamesEqual(path, "/"))
    {
        return Fat32SetStatus(volume, OrynFat32StatusPathInvalid);
    }
    if (!OrynFat32FindPath(volume, path, &info))
    {
        return Fat32SetStatus(volume, OrynFat32StatusNotFound);
    }
    if ((info.Attributes & ORYN_FAT32_ATTR_DIRECTORY) != 0U &&
        !Fat32DirectoryIsEmpty(volume, info.FirstCluster))
    {
        return Fat32SetStatus(volume, OrynFat32StatusDirectoryNotEmpty);
    }
    if (info.FirstCluster >= 2U && !Fat32FreeClusterChain(volume, info.FirstCluster))
    {
        return 0;
    }
    if (!OrynFat32ReadCluster(volume, info.DirectoryCluster, cluster_data))
    {
        return 0;
    }
    cluster_data[info.DirectoryOffset] = 0xE5U;
    return OrynFat32WriteCluster(volume, info.DirectoryCluster, cluster_data);
}
