#include "KernelFat32Internal.h"

int OrynFat32CreateDirectory(OrynFat32Volume* volume, const char* path)
{
    uint32_t parent;
    uint32_t offset;
    uint32_t cluster;
    char leaf[256];
    uint8_t sector[ORYN_FAT32_SECTOR_SIZE];
    OrynFat32FileInfo info;

    if (!Fat32ResolveParent(volume, path, &parent, leaf, sizeof(leaf)))
    {
        return 0;
    }
    if (Fat32FindInDirectory(volume, parent, leaf, 0))
    {
        return 0;
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
    return Fat32WriteDirectoryEntry(volume, parent, offset, &info);
}

int OrynFat32Delete(OrynFat32Volume* volume, const char* path)
{
    OrynFat32FileInfo info;
    uint8_t cluster_data[ORYN_FAT32_MAX_CLUSTER_BYTES];

    if (!OrynFat32FindPath(volume, path, &info))
    {
        return 0;
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
