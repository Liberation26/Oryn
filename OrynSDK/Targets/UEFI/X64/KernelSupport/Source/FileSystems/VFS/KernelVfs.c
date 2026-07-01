#include "KernelVfs.h"
#include "OrynString.h"

static OrynVfsMount gMounts[ORYN_VFS_MAX_MOUNTS];
static uint32_t gVfsLastStatus = OrynVfsStatusOk;

static int VfsSetStatus(uint32_t status)
{
    gVfsLastStatus = status;
    return 0;
}

uint32_t OrynVfsLastStatus(void)
{
    return gVfsLastStatus;
}

int OrynVfsNormalizePath(const char* path, char* output, uint32_t output_size)
{
    uint32_t input = 0U;
    uint32_t out = 0U;
    uint32_t component_start;
    if (path == 0 || output == 0 || output_size < 2U || path[0] != '/')
    {
        return 0;
    }
    output[out++] = '/';
    while (path[input] != 0)
    {
        while (path[input] == '/')
        {
            ++input;
        }
        if (path[input] == 0)
        {
            break;
        }
        component_start = input;
        while (path[input] != 0 && path[input] != '/')
        {
            if (path[input] == '\\')
            {
                return 0;
            }
            ++input;
        }
        if ((input - component_start == 1U && path[component_start] == '.') ||
            (input - component_start == 2U && path[component_start] == '.' && path[component_start + 1U] == '.'))
        {
            return 0;
        }
        if (out > 1U)
        {
            if (out + 1U >= output_size)
            {
                return 0;
            }
            output[out++] = '/';
        }
        while (component_start < input)
        {
            if (out + 1U >= output_size)
            {
                return 0;
            }
            output[out++] = path[component_start++];
        }
    }
    output[out] = 0;
    return 1;
}

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
    uint32_t best = 0xFFFFFFFFU;
    uint32_t best_length = 0U;
    for (index = 0; index < ORYN_VFS_MAX_MOUNTS; ++index)
    {
        if (gMounts[index].Mounted != 0U && VfsPrefixMatches(path, gMounts[index].Prefix))
        {
            uint32_t length = (uint32_t)OrynStrlen(gMounts[index].Prefix);
            if (best == 0xFFFFFFFFU || length > best_length)
            {
                best = index;
                best_length = length;
            }
        }
    }
    if (best != 0xFFFFFFFFU)
    {
        *local_path = VfsLocalPath(path, gMounts[best].Prefix);
        return &gMounts[best];
    }
    return 0;
}

static OrynVfsMount* VfsResolve(const char* path, const char** local_path, char* normalized, uint32_t normalized_size)
{
    OrynVfsMount* mount;
    if (!OrynVfsNormalizePath(path, normalized, normalized_size))
    {
        VfsSetStatus(OrynVfsStatusPathInvalid);
        return 0;
    }
    mount = VfsFindMount(normalized, local_path);
    if (mount == 0)
    {
        VfsSetStatus(OrynVfsStatusNoMount);
        return 0;
    }
    return mount;
}

void OrynVfsInit(void)
{
    OrynMemset(gMounts, 0, sizeof(gMounts));
    gVfsLastStatus = OrynVfsStatusOk;
}

int OrynVfsMountFat32(const char* prefix, OrynFat32Volume* volume)
{
    uint32_t index;
    char normalized[32];
    if (prefix == 0 || volume == 0 || volume->Mounted == 0U ||
        !OrynVfsNormalizePath(prefix, normalized, sizeof(normalized)))
    {
        return VfsSetStatus(OrynVfsStatusInvalidArgument);
    }
    for (index = 0; index < ORYN_VFS_MAX_MOUNTS; ++index)
    {
        if (gMounts[index].Mounted != 0U && OrynStrcmp(gMounts[index].Prefix, normalized) == 0)
        {
            return VfsSetStatus(OrynVfsStatusInvalidArgument);
        }
    }
    for (index = 0; index < ORYN_VFS_MAX_MOUNTS; ++index)
    {
        if (gMounts[index].Mounted == 0U)
        {
            OrynMemcpy(gMounts[index].Prefix, normalized, OrynStrlen(normalized) + 1U);
            gMounts[index].Fat32 = volume;
            gMounts[index].Mounted = 1U;
            gVfsLastStatus = OrynVfsStatusOk;
            return 1;
        }
    }
    return VfsSetStatus(OrynVfsStatusNoSpace);
}

int OrynVfsStatPath(const char* path, OrynVfsStat* stat)
{
    const char* local;
    char normalized[256];
    OrynVfsMount* mount;
    OrynFat32FileInfo info;
    if (stat == 0)
    {
        return VfsSetStatus(OrynVfsStatusInvalidArgument);
    }
    mount = VfsResolve(path, &local, normalized, sizeof(normalized));
    if (mount == 0 || !OrynFat32FindPath(mount->Fat32, local, &info))
    {
        return mount == 0 ? 0 : VfsSetStatus(OrynVfsStatusFsError);
    }
    stat->Type = (info.Attributes & ORYN_FAT32_ATTR_DIRECTORY) != 0U ? OrynVfsNodeDirectory : OrynVfsNodeFile;
    stat->SizeBytes = info.SizeBytes;
    stat->FirstCluster = info.FirstCluster;
    gVfsLastStatus = OrynVfsStatusOk;
    return 1;
}

int OrynVfsReadFile(const char* path, void* buffer, uint32_t capacity, uint32_t* bytes_read)
{
    const char* local;
    char normalized[256];
    OrynVfsMount* mount = VfsResolve(path, &local, normalized, sizeof(normalized));
    if (mount == 0 || !OrynFat32ReadFile(mount->Fat32, local, buffer, capacity, bytes_read))
    {
        return mount == 0 ? 0 : VfsSetStatus(OrynVfsStatusFsError);
    }
    gVfsLastStatus = OrynVfsStatusOk;
    return 1;
}

int OrynVfsWriteFile(const char* path, const void* buffer, uint32_t size)
{
    const char* local;
    char normalized[256];
    OrynVfsMount* mount = VfsResolve(path, &local, normalized, sizeof(normalized));
    if (mount == 0 || !OrynFat32WriteFile(mount->Fat32, local, buffer, size))
    {
        return mount == 0 ? 0 : VfsSetStatus(OrynVfsStatusFsError);
    }
    gVfsLastStatus = OrynVfsStatusOk;
    return 1;
}

int OrynVfsCreateDirectory(const char* path)
{
    const char* local;
    char normalized[256];
    OrynVfsMount* mount = VfsResolve(path, &local, normalized, sizeof(normalized));
    if (mount == 0 || !OrynFat32CreateDirectory(mount->Fat32, local))
    {
        return mount == 0 ? 0 : VfsSetStatus(OrynVfsStatusFsError);
    }
    gVfsLastStatus = OrynVfsStatusOk;
    return 1;
}

int OrynVfsDelete(const char* path)
{
    const char* local;
    char normalized[256];
    OrynVfsMount* mount = VfsResolve(path, &local, normalized, sizeof(normalized));
    if (mount == 0 || !OrynFat32Delete(mount->Fat32, local))
    {
        return mount == 0 ? 0 : VfsSetStatus(OrynVfsStatusFsError);
    }
    gVfsLastStatus = OrynVfsStatusOk;
    return 1;
}

int OrynVfsListDirectory(const char* path, OrynFat32FileInfo* entries, uint32_t max_entries, uint32_t* count)
{
    const char* local;
    char normalized[256];
    OrynVfsMount* mount = VfsResolve(path, &local, normalized, sizeof(normalized));
    if (mount == 0 || !OrynFat32ListDirectory(mount->Fat32, local, entries, max_entries, count))
    {
        return mount == 0 ? 0 : VfsSetStatus(OrynVfsStatusFsError);
    }
    gVfsLastStatus = OrynVfsStatusOk;
    return 1;
}
