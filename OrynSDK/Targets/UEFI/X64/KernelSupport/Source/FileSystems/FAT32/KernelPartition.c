#include "KernelPartition.h"
#include "OrynString.h"

static uint16_t PartRead16(const uint8_t* data, uint32_t offset)
{
    return (uint16_t)data[offset] | ((uint16_t)data[offset + 1U] << 8U);
}

static uint32_t PartRead32(const uint8_t* data, uint32_t offset)
{
    return (uint32_t)data[offset] |
        ((uint32_t)data[offset + 1U] << 8U) |
        ((uint32_t)data[offset + 2U] << 16U) |
        ((uint32_t)data[offset + 3U] << 24U);
}

static uint64_t PartRead64(const uint8_t* data, uint32_t offset)
{
    return (uint64_t)PartRead32(data, offset) |
        ((uint64_t)PartRead32(data, offset + 4U) << 32U);
}

static int PartGuidIsZero(const uint8_t* guid)
{
    for (uint32_t index = 0U; index < 16U; ++index)
    {
        if (guid[index] != 0U)
        {
            return 0;
        }
    }
    return 1;
}

static int PartSignatureValid(const uint8_t* sector)
{
    return PartRead16(sector, 510U) == 0xAA55U;
}

void OrynKernelPartitionTableInit(OrynKernelPartitionTable* table)
{
    if (table != 0)
    {
        OrynMemset(table, 0, sizeof(*table));
    }
}

const OrynKernelPartitionEntry* OrynKernelPartitionGetEntry(
    const OrynKernelPartitionTable* table,
    uint32_t index)
{
    if (table == 0 || index >= table->EntryCount)
    {
        return 0;
    }
    return &table->Entries[index];
}

static int PartAddMbrEntry(
    OrynKernelPartitionTable* table,
    const uint8_t* entry,
    uint32_t index)
{
    OrynKernelPartitionEntry* out;
    uint32_t type = entry[4];
    uint32_t first = PartRead32(entry, 8U);
    uint32_t count = PartRead32(entry, 12U);
    if (type == 0U || count == 0U || table->EntryCount >= ORYN_KERNEL_PARTITION_MAX_ENTRIES)
    {
        return 0;
    }
    out = &table->Entries[table->EntryCount];
    out->Present = 1U;
    out->Index = index;
    out->MbrType = type;
    out->FirstLba = (uint64_t)first;
    out->BlockCount = (uint64_t)count;
    table->EntryCount += 1U;
    return 1;
}

int OrynKernelPartitionParseMbr(OrynKernelBlockDevice* device, OrynKernelPartitionTable* table)
{
    uint8_t sector[ORYN_KERNEL_BLOCK_MAX_SECTOR_SIZE];
    if (device == 0 || table == 0 || !OrynKernelBlockReadSectors64(device, 0ULL, 1U, sector))
    {
        return 0;
    }
    if (!PartSignatureValid(sector))
    {
        return 0;
    }
    table->Type = OrynKernelPartitionTableMbr;
    table->Valid = 1U;
    table->MbrParsed = 1U;
    for (uint32_t index = 0U; index < 4U; ++index)
    {
        (void)PartAddMbrEntry(table, sector + 446U + (index * 16U), index);
    }
    return table->EntryCount > 0U;
}

static int PartGptHeaderValid(const uint8_t* header)
{
    return header[0] == 'E' && header[1] == 'F' && header[2] == 'I' && header[3] == ' ' &&
        header[4] == 'P' && header[5] == 'A' && header[6] == 'R' && header[7] == 'T';
}

static int PartAddGptEntry(OrynKernelPartitionTable* table, const uint8_t* entry, uint32_t index)
{
    OrynKernelPartitionEntry* out;
    if (PartGuidIsZero(entry) || table->EntryCount >= ORYN_KERNEL_PARTITION_MAX_ENTRIES)
    {
        return 0;
    }
    out = &table->Entries[table->EntryCount];
    out->Present = 1U;
    out->Index = index;
    out->FirstLba = PartRead64(entry, 32U);
    out->BlockCount = PartRead64(entry, 40U) - out->FirstLba + 1ULL;
    OrynMemcpy(out->TypeGuid, entry, 16U);
    OrynMemcpy(out->UniqueGuid, entry + 16U, 16U);
    table->EntryCount += 1U;
    return 1;
}

int OrynKernelPartitionParseGpt(OrynKernelBlockDevice* device, OrynKernelPartitionTable* table)
{
    uint8_t header[ORYN_KERNEL_BLOCK_MAX_SECTOR_SIZE];
    uint8_t sector[ORYN_KERNEL_BLOCK_MAX_SECTOR_SIZE];
    uint64_t entry_lba;
    uint32_t entry_count;
    uint32_t entry_size;
    if (device == 0 || table == 0 || !OrynKernelBlockReadSectors64(device, 1ULL, 1U, header))
    {
        return 0;
    }
    if (!PartGptHeaderValid(header))
    {
        return 0;
    }
    entry_lba = PartRead64(header, 72U);
    entry_count = PartRead32(header, 80U);
    entry_size = PartRead32(header, 84U);
    if (entry_size < 128U || entry_size > device->BytesPerSector)
    {
        return 0;
    }
    table->Type = OrynKernelPartitionTableGpt;
    table->Valid = 1U;
    table->GptParsed = 1U;
    for (uint32_t index = 0U; index < entry_count && index < ORYN_KERNEL_PARTITION_MAX_ENTRIES; ++index)
    {
        uint64_t lba = entry_lba + ((uint64_t)index * entry_size) / device->BytesPerSector;
        uint32_t offset = ((uint32_t)index * entry_size) % device->BytesPerSector;
        if (!OrynKernelBlockReadSectors64(device, lba, 1U, sector))
        {
            return table->EntryCount > 0U;
        }
        (void)PartAddGptEntry(table, sector + offset, index);
    }
    return table->EntryCount > 0U;
}

int OrynKernelPartitionParse(OrynKernelBlockDevice* device, OrynKernelPartitionTable* table)
{
    if (table == 0)
    {
        return 0;
    }
    OrynKernelPartitionTableInit(table);
    table->ParseAttempts = 1U;
    if (OrynKernelPartitionParseGpt(device, table) || OrynKernelPartitionParseMbr(device, table))
    {
        return 1;
    }
    table->ParseFailures = 1U;
    return 0;
}
