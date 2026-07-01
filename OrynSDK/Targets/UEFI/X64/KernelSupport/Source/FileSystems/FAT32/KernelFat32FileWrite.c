#include "KernelFat32Internal.h"

static int Fat32WriteChain(OrynFat32Volume* volume, uint32_t start_cluster, const uint8_t* input, uint32_t size)
{
    uint8_t cluster_data[ORYN_FAT32_MAX_CLUSTER_BYTES];
    uint32_t current = start_cluster;
    uint32_t copied = 0;
    uint32_t cluster_size = Fat32ClusterSize(volume);

    while (copied < size)
    {
        uint32_t chunk = size - copied;
        uint32_t next;
        if (chunk > cluster_size)
        {
            chunk = cluster_size;
        }
        OrynMemset(cluster_data, 0, sizeof(cluster_data));
        OrynMemcpy(cluster_data, input + copied, chunk);
        if (!OrynFat32WriteCluster(volume, current, cluster_data))
        {
            return 0;
        }
        copied += chunk;
        if (copied >= size)
        {
            break;
        }
        if (!OrynFat32GetFatEntry(volume, current, &next) || Fat32IsEndOfChain(next))
        {
            if (!Fat32AppendCluster(volume, start_cluster, &next))
            {
                return 0;
            }
        }
        current = next;
    }
    return 1;
}

static uint32_t Fat32ClustersNeeded(OrynFat32Volume* volume, uint32_t size)
{
    uint32_t cluster_size = Fat32ClusterSize(volume);
    if (size == 0U)
    {
        return 0U;
    }
    return (size + cluster_size - 1U) / cluster_size;
}

int OrynFat32WriteFile(OrynFat32Volume* volume, const char* path, const void* buffer, uint32_t size)
{
    OrynFat32FileInfo info;
    uint32_t clusters_needed;
    uint32_t current;
    uint32_t count;

    if (volume == 0 || path == 0 || (size > 0U && buffer == 0) || !Fat32PathIsSafeShortName(path))
    {
        return Fat32SetStatus(volume, OrynFat32StatusPathInvalid);
    }
    if (!OrynFat32FindPath(volume, path, &info) ||
        (info.Attributes & ORYN_FAT32_ATTR_DIRECTORY) != 0U)
    {
        return Fat32SetStatus(volume, OrynFat32StatusNotFound);
    }

    clusters_needed = Fat32ClustersNeeded(volume, size);
    if (clusters_needed == 0U)
    {
        if (info.FirstCluster >= 2U && !Fat32FreeClusterChain(volume, info.FirstCluster))
        {
            return 0;
        }
        info.FirstCluster = 0U;
        info.SizeBytes = 0U;
        return Fat32WriteDirectoryEntry(volume, info.DirectoryCluster, info.DirectoryOffset, &info);
    }

    if (info.FirstCluster < 2U && !Fat32AllocateCluster(volume, &info.FirstCluster))
    {
        return 0;
    }

    current = info.FirstCluster;
    for (count = 1U; count < clusters_needed; ++count)
    {
        uint32_t next;
        if (!OrynFat32GetFatEntry(volume, current, &next))
        {
            return 0;
        }
        if (Fat32IsEndOfChain(next))
        {
            if (!Fat32AppendCluster(volume, info.FirstCluster, &next))
            {
                return 0;
            }
        }
        current = next;
    }

    if (!Fat32TruncateChain(volume, info.FirstCluster, clusters_needed))
    {
        return 0;
    }
    if (!Fat32WriteChain(volume, info.FirstCluster, (const uint8_t*)buffer, size))
    {
        return 0;
    }

    info.SizeBytes = size;
    return Fat32WriteDirectoryEntry(volume, info.DirectoryCluster, info.DirectoryOffset, &info);
}

int OrynFat32CreateFile(OrynFat32Volume* volume, const char* path, const void* buffer, uint32_t size)
{
    uint32_t parent;
    uint32_t offset;
    char leaf[256];
    uint8_t sector[ORYN_FAT32_SECTOR_SIZE];
    OrynFat32FileInfo info;

    if (volume == 0 || path == 0 || (size > 0U && buffer == 0) || !Fat32PathIsSafeShortName(path))
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

    OrynMemset(&info, 0, sizeof(info));
    OrynMemcpy(info.Name, leaf, OrynStrlen(leaf) + 1U);
    info.Attributes = ORYN_FAT32_ATTR_ARCHIVE;
    info.SizeBytes = size;
    info.DirectoryCluster = parent;
    info.DirectoryOffset = offset;
    if (size > 0U && !Fat32AllocateCluster(volume, &info.FirstCluster))
    {
        return 0;
    }
    if (!Fat32WriteDirectoryEntry(volume, parent, offset, &info))
    {
        if (info.FirstCluster >= 2U)
        {
            (void)Fat32FreeClusterChain(volume, info.FirstCluster);
        }
        return 0;
    }
    return size == 0U || OrynFat32WriteFile(volume, path, buffer, size);
}
