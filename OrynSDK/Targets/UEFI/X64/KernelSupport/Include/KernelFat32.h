#ifndef ORYN_KERNEL_FAT32_H
#define ORYN_KERNEL_FAT32_H

#include "KernelBlockDevice.h"

#define ORYN_FAT32_SECTOR_SIZE 512U
#define ORYN_FAT32_ATTR_READ_ONLY 0x01U
#define ORYN_FAT32_ATTR_HIDDEN 0x02U
#define ORYN_FAT32_ATTR_SYSTEM 0x04U
#define ORYN_FAT32_ATTR_VOLUME_ID 0x08U
#define ORYN_FAT32_ATTR_DIRECTORY 0x10U
#define ORYN_FAT32_ATTR_ARCHIVE 0x20U
#define ORYN_FAT32_ATTR_LONG_NAME 0x0FU

typedef enum OrynFat32Status
{
    OrynFat32StatusOk = 0,
    OrynFat32StatusInvalidArgument = 1,
    OrynFat32StatusIoError = 2,
    OrynFat32StatusInvalidBootSector = 3,
    OrynFat32StatusUnsupportedFormat = 4,
    OrynFat32StatusInvalidCluster = 5,
    OrynFat32StatusPathInvalid = 6,
    OrynFat32StatusNotFound = 7,
    OrynFat32StatusAlreadyExists = 8,
    OrynFat32StatusNoSpace = 9,
    OrynFat32StatusDirectoryNotEmpty = 10,
    OrynFat32StatusReadOnly = 11
} OrynFat32Status;

typedef struct OrynFat32BootInfo
{
    uint32_t BytesPerSector;
    uint32_t SectorsPerCluster;
    uint32_t ReservedSectorCount;
    uint32_t FatCount;
    uint32_t FatSizeSectors;
    uint32_t RootCluster;
    uint32_t TotalSectors;
    uint32_t FirstFatSector;
    uint32_t FirstDataSector;
    uint32_t ClusterCount;
    uint32_t FsInfoSector;
    uint32_t BackupBootSector;
    uint32_t FreeClusterHint;
    uint32_t NextFreeClusterHint;
    uint32_t ValidFsInfo;
} OrynFat32BootInfo;

typedef struct OrynFat32FileInfo
{
    char Name[256];
    uint8_t Attributes;
    uint32_t FirstCluster;
    uint32_t SizeBytes;
    uint32_t DirectoryCluster;
    uint32_t DirectoryOffset;
} OrynFat32FileInfo;

typedef struct OrynFat32Volume
{
    OrynKernelBlockDevice* Device;
    OrynFat32BootInfo Info;
    uint8_t Sector[ORYN_FAT32_SECTOR_SIZE];
    uint32_t Mounted;
    uint32_t ReadOnly;
    uint32_t LastStatus;
} OrynFat32Volume;

int OrynFat32Mount(OrynFat32Volume* volume, OrynKernelBlockDevice* device);
int OrynFat32ReadCluster(OrynFat32Volume* volume, uint32_t cluster, void* buffer);
int OrynFat32WriteCluster(OrynFat32Volume* volume, uint32_t cluster, const void* buffer);
int OrynFat32GetFatEntry(OrynFat32Volume* volume, uint32_t cluster, uint32_t* value);
int OrynFat32SetFatEntry(OrynFat32Volume* volume, uint32_t cluster, uint32_t value);
int OrynFat32FindPath(OrynFat32Volume* volume, const char* path, OrynFat32FileInfo* info);
int OrynFat32ReadFile(OrynFat32Volume* volume, const char* path, void* buffer, uint32_t capacity, uint32_t* bytes_read);
int OrynFat32WriteFile(OrynFat32Volume* volume, const char* path, const void* buffer, uint32_t size);
int OrynFat32CreateFile(OrynFat32Volume* volume, const char* path, const void* buffer, uint32_t size);
int OrynFat32CreateDirectory(OrynFat32Volume* volume, const char* path);
int OrynFat32Delete(OrynFat32Volume* volume, const char* path);
int OrynFat32ListDirectory(OrynFat32Volume* volume, const char* path, OrynFat32FileInfo* entries, uint32_t max_entries, uint32_t* count);
int OrynFat32FormatMemoryImage(uint8_t* image, uint32_t sector_count, const char* volume_label);
int OrynFat32CheckVolume(OrynFat32Volume* volume);
int OrynFat32ValidatePath(const char* path);
int OrynFat32Flush(OrynFat32Volume* volume);
int OrynFat32GetFreeClusterEstimate(OrynFat32Volume* volume, uint32_t* free_clusters);
uint32_t OrynFat32LastStatus(const OrynFat32Volume* volume);

#endif
