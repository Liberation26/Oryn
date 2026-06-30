#include "KernelFat32Internal.h"

int OrynFat32FindPath(OrynFat32Volume* volume, const char* path, OrynFat32FileInfo* info)
{
    const char* cursor = path;
    Fat32PathPart part;
    uint32_t directory_cluster;
    OrynFat32FileInfo current;

    if (volume == 0 || path == 0 || info == 0)
    {
        return 0;
    }

    if (Fat32NamesEqual(path, "/") || path[0] == 0)
    {
        OrynMemset(info, 0, sizeof(OrynFat32FileInfo));
        info->Name[0] = '/';
        info->Name[1] = 0;
        info->Attributes = ORYN_FAT32_ATTR_DIRECTORY;
        info->FirstCluster = volume->Info.RootCluster;
        return 1;
    }

    directory_cluster = volume->Info.RootCluster;
    while (Fat32NextPathPart(&cursor, &part))
    {
        if (!Fat32FindInDirectory(volume, directory_cluster, part.Text, &current))
        {
            return 0;
        }

        if (part.IsLast)
        {
            *info = current;
            return 1;
        }

        if ((current.Attributes & ORYN_FAT32_ATTR_DIRECTORY) == 0U)
        {
            return 0;
        }
        directory_cluster = current.FirstCluster;
    }

    return 0;
}

int Fat32ResolveParent(OrynFat32Volume* volume, const char* path, uint32_t* parent_cluster, char* leaf, uint32_t leaf_size)
{
    const char* cursor = path;
    Fat32PathPart part;
    OrynFat32FileInfo current;
    uint32_t directory_cluster;

    if (volume == 0 || path == 0 || parent_cluster == 0 || leaf == 0 || leaf_size == 0U)
    {
        return 0;
    }

    directory_cluster = volume->Info.RootCluster;
    while (Fat32NextPathPart(&cursor, &part))
    {
        if (part.IsLast)
        {
            uint32_t index = 0;
            while (part.Text[index] != 0 && index + 1U < leaf_size)
            {
                leaf[index] = part.Text[index];
                ++index;
            }
            leaf[index] = 0;
            *parent_cluster = directory_cluster;
            return index > 0U;
        }

        if (!Fat32FindInDirectory(volume, directory_cluster, part.Text, &current) ||
            (current.Attributes & ORYN_FAT32_ATTR_DIRECTORY) == 0U)
        {
            return 0;
        }
        directory_cluster = current.FirstCluster;
    }

    return 0;
}
