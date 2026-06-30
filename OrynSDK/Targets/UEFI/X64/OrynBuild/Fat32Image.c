#include "OrynBuild.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ORYN_DISK_SIZE (64U * 1024U * 1024U)
#define ORYN_PARTITION_LBA 2048U
#define ORYN_SECTOR_SIZE 512U
#define ORYN_RESERVED_SECTORS 32U
#define ORYN_FAT_COUNT 2U
#define ORYN_CLUSTER_SECTORS 1U
#define ORYN_ROOT_CLUSTER 2U
#define ORYN_END_OF_CHAIN 0x0FFFFFFFU

typedef struct OrynFat32Writer
{
    uint8_t* image;
    uint32_t partition_sectors;
    uint32_t fat_sectors;
    uint32_t first_fat_lba;
    uint32_t first_data_lba;
    uint32_t next_cluster;
    uint32_t cluster_count;
} OrynFat32Writer;

static void WriteLe16(uint8_t* target, uint16_t value)
{
    target[0] = (uint8_t)(value & 0xFFU);
    target[1] = (uint8_t)((value >> 8) & 0xFFU);
}

static void WriteLe32(uint8_t* target, uint32_t value)
{
    target[0] = (uint8_t)(value & 0xFFU);
    target[1] = (uint8_t)((value >> 8) & 0xFFU);
    target[2] = (uint8_t)((value >> 16) & 0xFFU);
    target[3] = (uint8_t)((value >> 24) & 0xFFU);
}

static uint32_t DivideRoundUp(uint32_t value, uint32_t divisor)
{
    return (value + divisor - 1U) / divisor;
}

static uint8_t* Sector(OrynFat32Writer* writer, uint32_t lba)
{
    return writer->image + ((uint64_t)lba * ORYN_SECTOR_SIZE);
}

static uint8_t* Cluster(OrynFat32Writer* writer, uint32_t cluster)
{
    uint32_t data_sector = writer->first_data_lba + ((cluster - 2U) * ORYN_CLUSTER_SECTORS);
    return Sector(writer, data_sector);
}

static void ComputeFatLayout(OrynFat32Writer* writer)
{
    writer->partition_sectors = (ORYN_DISK_SIZE / ORYN_SECTOR_SIZE) - ORYN_PARTITION_LBA;
    writer->fat_sectors = 1U;

    for (;;)
    {
        uint32_t data_sectors = writer->partition_sectors - ORYN_RESERVED_SECTORS -
            (ORYN_FAT_COUNT * writer->fat_sectors);
        uint32_t clusters = data_sectors / ORYN_CLUSTER_SECTORS;
        uint32_t required_fat_sectors = DivideRoundUp((clusters + 2U) * 4U, ORYN_SECTOR_SIZE);

        if (required_fat_sectors == writer->fat_sectors)
        {
            writer->cluster_count = clusters;
            break;
        }

        writer->fat_sectors = required_fat_sectors;
    }

    writer->first_fat_lba = ORYN_PARTITION_LBA + ORYN_RESERVED_SECTORS;
    writer->first_data_lba = writer->first_fat_lba + (ORYN_FAT_COUNT * writer->fat_sectors);
    writer->next_cluster = ORYN_ROOT_CLUSTER + 1U;
}

static void WriteMbr(OrynFat32Writer* writer)
{
    uint8_t* mbr = Sector(writer, 0);
    mbr[446] = 0x00U;
    mbr[447] = 0xFEU;
    mbr[448] = 0xFFU;
    mbr[449] = 0xFFU;
    mbr[450] = 0xEFU;
    mbr[451] = 0xFEU;
    mbr[452] = 0xFFU;
    mbr[453] = 0xFFU;
    WriteLe32(mbr + 454, ORYN_PARTITION_LBA);
    WriteLe32(mbr + 458, writer->partition_sectors);
    mbr[510] = 0x55U;
    mbr[511] = 0xAAU;
}

static void WriteBootSector(OrynFat32Writer* writer, uint32_t lba)
{
    uint8_t* boot = Sector(writer, lba);
    boot[0] = 0xEBU;
    boot[1] = 0x58U;
    boot[2] = 0x90U;
    memcpy(boot + 3, "ORYN    ", 8);
    WriteLe16(boot + 11, ORYN_SECTOR_SIZE);
    boot[13] = ORYN_CLUSTER_SECTORS;
    WriteLe16(boot + 14, ORYN_RESERVED_SECTORS);
    boot[16] = ORYN_FAT_COUNT;
    WriteLe16(boot + 17, 0);
    WriteLe16(boot + 19, 0);
    boot[21] = 0xF8U;
    WriteLe16(boot + 22, 0);
    WriteLe16(boot + 24, 63);
    WriteLe16(boot + 26, 255);
    WriteLe32(boot + 28, ORYN_PARTITION_LBA);
    WriteLe32(boot + 32, writer->partition_sectors);
    WriteLe32(boot + 36, writer->fat_sectors);
    WriteLe16(boot + 40, 0);
    WriteLe16(boot + 42, 0);
    WriteLe32(boot + 44, ORYN_ROOT_CLUSTER);
    WriteLe16(boot + 48, 1);
    WriteLe16(boot + 50, 6);
    boot[64] = 0x80U;
    boot[66] = 0x29U;
    WriteLe32(boot + 67, 0x4F52594EU);
    memcpy(boot + 71, "ORYN ESP   ", 11);
    memcpy(boot + 82, "FAT32   ", 8);
    boot[510] = 0x55U;
    boot[511] = 0xAAU;
}

static void WriteFsInfo(OrynFat32Writer* writer, uint32_t lba)
{
    uint8_t* fsinfo = Sector(writer, lba);
    WriteLe32(fsinfo + 0, 0x41615252U);
    WriteLe32(fsinfo + 484, 0x61417272U);
    WriteLe32(fsinfo + 488, 0xFFFFFFFFU);
    WriteLe32(fsinfo + 492, writer->next_cluster);
    WriteLe32(fsinfo + 508, 0xAA550000U);
}

static uint32_t* FatEntry(OrynFat32Writer* writer, uint32_t fat_index, uint32_t cluster)
{
    uint8_t* fat = Sector(writer, writer->first_fat_lba + (fat_index * writer->fat_sectors));
    return (uint32_t*)(void*)(fat + (cluster * 4U));
}

static void SetFat(OrynFat32Writer* writer, uint32_t cluster, uint32_t value)
{
    for (uint32_t fat = 0; fat < ORYN_FAT_COUNT; ++fat)
    {
        *FatEntry(writer, fat, cluster) = value;
    }
}

static void InitializeFat(OrynFat32Writer* writer)
{
    SetFat(writer, 0, 0x0FFFFFF8U);
    SetFat(writer, 1, ORYN_END_OF_CHAIN);
    SetFat(writer, ORYN_ROOT_CLUSTER, ORYN_END_OF_CHAIN);
}

static uint32_t AllocateCluster(OrynFat32Writer* writer)
{
    uint32_t cluster = writer->next_cluster++;
    SetFat(writer, cluster, ORYN_END_OF_CHAIN);
    memset(Cluster(writer, cluster), 0, ORYN_SECTOR_SIZE * ORYN_CLUSTER_SECTORS);
    return cluster;
}

static uint32_t AllocateChain(OrynFat32Writer* writer, uint32_t byte_count)
{
    uint32_t clusters = DivideRoundUp(byte_count == 0 ? 1U : byte_count,
        ORYN_SECTOR_SIZE * ORYN_CLUSTER_SECTORS);
    uint32_t first = 0;
    uint32_t previous = 0;

    for (uint32_t index = 0; index < clusters; ++index)
    {
        uint32_t current = AllocateCluster(writer);
        if (first == 0)
        {
            first = current;
        }
        if (previous != 0)
        {
            SetFat(writer, previous, current);
        }
        previous = current;
    }

    SetFat(writer, previous, ORYN_END_OF_CHAIN);
    return first;
}

static void FillName(uint8_t* entry, const char* base, const char* extension)
{
    memset(entry, ' ', 11);
    for (int index = 0; index < 8 && base[index] != 0; ++index)
    {
        entry[index] = (uint8_t)base[index];
    }
    for (int index = 0; index < 3 && extension[index] != 0; ++index)
    {
        entry[8 + index] = (uint8_t)extension[index];
    }
}

static void WriteDirEntry(uint8_t* directory, int entry_index, const char* base, const char* extension,
    uint8_t attributes, uint32_t cluster, uint32_t size)
{
    uint8_t* entry = directory + (entry_index * 32);
    FillName(entry, base, extension);
    entry[11] = attributes;
    WriteLe16(entry + 20, (uint16_t)(cluster >> 16));
    WriteLe16(entry + 26, (uint16_t)(cluster & 0xFFFFU));
    WriteLe32(entry + 28, size);
}

static void WriteDotEntries(uint8_t* directory, uint32_t self_cluster, uint32_t parent_cluster)
{
    WriteDirEntry(directory, 0, ".", "", 0x10U, self_cluster, 0);
    WriteDirEntry(directory, 1, "..", "", 0x10U, parent_cluster, 0);
}

static int ReadWholeFile(const char* path, uint8_t** out_data, uint32_t* out_size)
{
    FILE* file = fopen(path, "rb");
    if (file == 0)
    {
        return 0;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return 0;
    }
    long size = ftell(file);
    rewind(file);

    if (size < 0 || size > 32L * 1024L * 1024L)
    {
        fclose(file);
        return 0;
    }

    uint8_t* data = (uint8_t*)malloc((size_t)size == 0 ? 1U : (size_t)size);
    if (data == 0)
    {
        fclose(file);
        return 0;
    }

    if (fread(data, 1, (size_t)size, file) != (size_t)size)
    {
        free(data);
        fclose(file);
        return 0;
    }

    fclose(file);
    *out_data = data;
    *out_size = (uint32_t)size;
    return 1;
}

static void WriteFileData(OrynFat32Writer* writer, uint32_t first_cluster, const uint8_t* data, uint32_t size)
{
    uint32_t cluster = first_cluster;
    uint32_t written = 0;
    while (cluster < ORYN_END_OF_CHAIN && written < size)
    {
        uint32_t copy_size = size - written;
        if (copy_size > ORYN_SECTOR_SIZE * ORYN_CLUSTER_SECTORS)
        {
            copy_size = ORYN_SECTOR_SIZE * ORYN_CLUSTER_SECTORS;
        }
        memcpy(Cluster(writer, cluster), data + written, copy_size);
        written += copy_size;
        cluster = *FatEntry(writer, 0, cluster) & 0x0FFFFFFFU;
    }
}

static int WriteImageFile(const char* image_path, const uint8_t* data)
{
    FILE* file = fopen(image_path, "wb");
    if (file == 0)
    {
        return 0;
    }
    int ok = fwrite(data, 1, ORYN_DISK_SIZE, file) == ORYN_DISK_SIZE;
    fclose(file);
    return ok;
}

int OrynCreateFat32EspImage(
    const char* boot_efi_path,
    const char* kernel_elf_path,
    const char* font_ttf_path,
    const char* image_path,
    const char* kernel_directory_name,
    const char* kernel_file_name)
{
    uint8_t* boot_data = 0;
    uint8_t* kernel_data = 0;
    uint8_t* font_data = 0;
    uint32_t boot_size = 0;
    uint32_t kernel_size = 0;
    uint32_t font_size = 0;

    if (!ReadWholeFile(boot_efi_path, &boot_data, &boot_size) ||
        !ReadWholeFile(kernel_elf_path, &kernel_data, &kernel_size))
    {
        free(boot_data);
        free(kernel_data);
        OrynLogFail("Could not read BOOTX64.EFI or the OS-named kernel ELF for FAT32 image.");
        return 0;
    }

    if (font_ttf_path != 0 && font_ttf_path[0] != 0)
    {
        if (!ReadWholeFile(font_ttf_path, &font_data, &font_size))
        {
            free(boot_data);
            free(kernel_data);
            OrynLogFail("Could not read TTF font for FAT32 image.");
            return 0;
        }
    }

    char kernel_directory_base[16];
    OrynMakeFatDirectoryName(kernel_directory_base, sizeof(kernel_directory_base), kernel_directory_name);

    char kernel_file_without_extension[256];
    snprintf(kernel_file_without_extension, sizeof(kernel_file_without_extension), "%s", kernel_file_name);
    char* kernel_file_dot = strchr(kernel_file_without_extension, '.');
    if (kernel_file_dot != 0)
    {
        *kernel_file_dot = 0;
    }

    char kernel_file_base[16];
    OrynMakeFatDirectoryName(kernel_file_base, sizeof(kernel_file_base), kernel_file_without_extension);

    OrynFat32Writer writer;
    memset(&writer, 0, sizeof(writer));
    writer.image = (uint8_t*)calloc(1, ORYN_DISK_SIZE);
    if (writer.image == 0)
    {
        free(boot_data);
        free(kernel_data);
        free(font_data);
        OrynLogFail("Could not allocate FAT32 image buffer.");
        return 0;
    }

    ComputeFatLayout(&writer);
    WriteMbr(&writer);
    WriteBootSector(&writer, ORYN_PARTITION_LBA);
    WriteBootSector(&writer, ORYN_PARTITION_LBA + 6U);
    WriteFsInfo(&writer, ORYN_PARTITION_LBA + 1U);
    InitializeFat(&writer);

    uint32_t efi_cluster = AllocateCluster(&writer);
    uint32_t boot_dir_cluster = AllocateCluster(&writer);
    uint32_t boot_file_cluster = AllocateChain(&writer, boot_size);
    uint32_t system_cluster = AllocateCluster(&writer);
    uint32_t kernel_dir_cluster = AllocateCluster(&writer);
    uint32_t kernel_file_cluster = AllocateChain(&writer, kernel_size);
    uint32_t fonts_dir_cluster = AllocateCluster(&writer);
    uint32_t font_file_cluster = 0;
    if (font_data != 0 && font_size != 0)
    {
        font_file_cluster = AllocateChain(&writer, font_size);
    }

    WriteDirEntry(Cluster(&writer, ORYN_ROOT_CLUSTER), 0, "EFI", "", 0x10U, efi_cluster, 0);
    WriteDirEntry(Cluster(&writer, ORYN_ROOT_CLUSTER), 1, "SYSTEM", "", 0x10U, system_cluster, 0);
    WriteDotEntries(Cluster(&writer, efi_cluster), efi_cluster, ORYN_ROOT_CLUSTER);
    WriteDirEntry(Cluster(&writer, efi_cluster), 2, "BOOT", "", 0x10U, boot_dir_cluster, 0);
    WriteDotEntries(Cluster(&writer, boot_dir_cluster), boot_dir_cluster, efi_cluster);
    WriteDirEntry(Cluster(&writer, boot_dir_cluster), 2, "BOOTX64", "EFI", 0x20U, boot_file_cluster, boot_size);
    WriteDotEntries(Cluster(&writer, system_cluster), system_cluster, ORYN_ROOT_CLUSTER);
    WriteDirEntry(Cluster(&writer, system_cluster), 2, kernel_directory_base, "", 0x10U, kernel_dir_cluster, 0);
    WriteDirEntry(Cluster(&writer, system_cluster), 3, "FONTS", "", 0x10U, fonts_dir_cluster, 0);
    WriteDotEntries(Cluster(&writer, kernel_dir_cluster), kernel_dir_cluster, system_cluster);
    WriteDirEntry(Cluster(&writer, kernel_dir_cluster), 2, kernel_file_base, "ELF", 0x20U, kernel_file_cluster, kernel_size);
    WriteDotEntries(Cluster(&writer, fonts_dir_cluster), fonts_dir_cluster, system_cluster);
    if (font_file_cluster != 0)
    {
        WriteDirEntry(Cluster(&writer, fonts_dir_cluster), 2, "ORYNSANS", "TTF", 0x20U, font_file_cluster, font_size);
    }

    WriteFileData(&writer, boot_file_cluster, boot_data, boot_size);
    WriteFileData(&writer, kernel_file_cluster, kernel_data, kernel_size);
    if (font_file_cluster != 0)
    {
        WriteFileData(&writer, font_file_cluster, font_data, font_size);
    }

    int ok = WriteImageFile(image_path, writer.image);
    free(writer.image);
    free(boot_data);
    free(kernel_data);
    free(font_data);
    return ok;
}
