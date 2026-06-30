#ifndef ORYN_KERNEL_VFS_H
#define ORYN_KERNEL_VFS_H

#include "KernelFat32.h"

#define ORYN_VFS_MAX_MOUNTS 4U
#define ORYN_VFS_MAX_HANDLES 16U

typedef enum OrynVfsNodeType
{
    OrynVfsNodeMissing = 0,
    OrynVfsNodeFile = 1,
    OrynVfsNodeDirectory = 2
} OrynVfsNodeType;

typedef struct OrynVfsStat
{
    OrynVfsNodeType Type;
    uint32_t SizeBytes;
    uint32_t FirstCluster;
} OrynVfsStat;

typedef struct OrynVfsMount
{
    char Prefix[32];
    OrynFat32Volume* Fat32;
} OrynVfsMount;

void OrynVfsInit(void);
int OrynVfsMountFat32(const char* prefix, OrynFat32Volume* volume);
int OrynVfsStatPath(const char* path, OrynVfsStat* stat);
int OrynVfsReadFile(const char* path, void* buffer, uint32_t capacity, uint32_t* bytes_read);
int OrynVfsWriteFile(const char* path, const void* buffer, uint32_t size);
int OrynVfsCreateDirectory(const char* path);
int OrynVfsDelete(const char* path);
int OrynVfsListDirectory(const char* path, OrynFat32FileInfo* entries, uint32_t max_entries, uint32_t* count);

#endif
