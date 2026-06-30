#include "KernelVfs.h"
#include "OrynString.h"

static OrynVfsMount gMounts[ORYN_VFS_MAX_MOUNTS];

static int VfsPrefixMatches(const char* path, const char* prefix)
{
    uint32_t index = 0;
    if (prefix[0] == '/' && prefix[1] == 0)
    {
        return path[0] == '/';
    }
    while (prefix[index] != 0)
    {
        if (path[index] != prefix[index])
        {
            return 0;
        }
        ++index;
    }
    return path[index] == 0 || path[index] == '/';
}

static const char* VfsLocalPath(const char* path, const char* prefix)
{
    uint32_t length = (uint32_t)OrynStrlen(prefix);
    if (prefix[0] == '/' && prefix[1] == 0)
    {
        return path;
    }
    if (path[length] == 0)
    {
        return "/";
    }
    return path + length;
}

static OrynVfsMount* VfsFindMount(const char* path, const char** local_path)
{
    uint32_t index;
    for (index = 0; index < ORYN_VFS_MAX_MOUNTS; ++index)
    {
        if (gMounts[index].Fat32 != 0 && VfsPrefixMatches(path, gMounts[index].Prefix))
        {
            *local_path = VfsLocalPath(path, gMounts[index].Prefix);
            return &gMounts[index];
        }
    }
    return 0;
}

void OrynVfsInit(void)
{
    OrynMemset(gMounts, 0, sizeof(gMounts));
}

int OrynVfsMountFat32(const char* prefix, OrynFat32Volume* volume)
{
    uint32_t index;
    if (prefix == 0 || volume == 0 || prefix[0] != '/')
    {
        return 0;
    }
    for (index = 0; index < ORYN_VFS_MAX_MOUNTS; ++index)
    {
        if (gMounts[index].Fat32 == 0)
        {
            OrynMemcpy(gMounts[index].Prefix, prefix, OrynStrlen(prefix) + 1U);
            gMounts[index].Fat32 = volume;
            return 1;
        }
    }
    return 0;
}

int OrynVfsStatPath(const char* path, OrynVfsStat* stat)
{
    const char* local;
    OrynVfsMount* mount = VfsFindMount(path, &local);
    OrynFat32FileInfo info;
    if (mount == 0 || stat == 0 || !OrynFat32FindPath(mount->Fat32, local, &info))
    {
        return 0;
    }
    stat->Type = (info.Attributes & ORYN_FAT32_ATTR_DIRECTORY) != 0U ? OrynVfsNodeDirectory : OrynVfsNodeFile;
    stat->SizeBytes = info.SizeBytes;
    stat->FirstCluster = info.FirstCluster;
    return 1;
}

int OrynVfsReadFile(const char* path, void* buffer, uint32_t capacity, uint32_t* bytes_read)
{
    const char* local;
    OrynVfsMount* mount = VfsFindMount(path, &local);
    return mount != 0 && OrynFat32ReadFile(mount->Fat32, local, buffer, capacity, bytes_read);
}

int OrynVfsWriteFile(const char* path, const void* buffer, uint32_t size)
{
    const char* local;
    OrynVfsMount* mount = VfsFindMount(path, &local);
    return mount != 0 && OrynFat32WriteFile(mount->Fat32, local, buffer, size);
}

int OrynVfsCreateDirectory(const char* path)
{
    const char* local;
    OrynVfsMount* mount = VfsFindMount(path, &local);
    return mount != 0 && OrynFat32CreateDirectory(mount->Fat32, local);
}

int OrynVfsDelete(const char* path)
{
    const char* local;
    OrynVfsMount* mount = VfsFindMount(path, &local);
    return mount != 0 && OrynFat32Delete(mount->Fat32, local);
}

int OrynVfsListDirectory(const char* path, OrynFat32FileInfo* entries, uint32_t max_entries, uint32_t* count)
{
    const char* local;
    OrynVfsMount* mount = VfsFindMount(path, &local);
    return mount != 0 && OrynFat32ListDirectory(mount->Fat32, local, entries, max_entries, count);
}
