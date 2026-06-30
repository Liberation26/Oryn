#include "KernelFat32Internal.h"

int OrynFat32ReadFile(OrynFat32Volume* volume, const char* path, void* buffer, uint32_t capacity, uint32_t* bytes_read)
{
    OrynFat32FileInfo info;
    uint8_t cluster_data[ORYN_FAT32_MAX_CLUSTER_BYTES];
    uint8_t* output = (uint8_t*)buffer;
    uint32_t current;
    uint32_t next;
    uint32_t copied = 0;
    uint32_t cluster_size;

    if (bytes_read == 0 || buffer == 0 || volume == 0)
    {
        return 0;
    }
    *bytes_read = 0;

    if (!OrynFat32FindPath(volume, path, &info) ||
        (info.Attributes & ORYN_FAT32_ATTR_DIRECTORY) != 0U)
    {
        return 0;
    }

    current = info.FirstCluster;
    cluster_size = Fat32ClusterSize(volume);
    while (current >= 2U && copied < info.SizeBytes && copied < capacity)
    {
        uint32_t remaining = info.SizeBytes - copied;
        uint32_t space = capacity - copied;
        uint32_t chunk = remaining < cluster_size ? remaining : cluster_size;
        chunk = chunk < space ? chunk : space;

        if (!OrynFat32ReadCluster(volume, current, cluster_data))
        {
            return 0;
        }
        OrynMemcpy(output + copied, cluster_data, chunk);
        copied += chunk;

        if (copied >= info.SizeBytes || copied >= capacity)
        {
            break;
        }
        if (!OrynFat32GetFatEntry(volume, current, &next) || Fat32IsEndOfChain(next))
        {
            break;
        }
        current = next;
    }

    *bytes_read = copied;
    return copied == info.SizeBytes || copied == capacity;
}
