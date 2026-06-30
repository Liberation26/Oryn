#ifndef ORYN_BOOT_INFO_H
#define ORYN_BOOT_INFO_H

#define ORYN_BOOTINFO_SIGNATURE 0x544F4F424E59524FULL
#define ORYN_BOOTINFO_VERSION 4U
#define ORYN_BOOTINFO_ABI_STABLE 1U
#define ORYN_BOOTINFO_ABI_MAJOR 1U
#define ORYN_BOOTINFO_ABI_MINOR 0U
#define ORYN_BOOTINFO_ABI_CURRENT_VERSION ORYN_BOOTINFO_VERSION
#define ORYN_BOOTINFO_ABI_MIN_COMPATIBLE_VERSION 4U
#define ORYN_BOOTINFO_ABI_CURRENT_SIZE 1672U
#define ORYN_BOOTINFO_ABI_MIN_COMPATIBLE_SIZE 1672U
#define ORYN_BOOTINFO_ABI_NAME "OrynBootInfo-v4-stable"
#define ORYN_BOOTINFO_ABI_PLAIN_C 1U
#define ORYN_BOOTINFO_X64_ENTRY_REGISTER_RDI 1U
#define ORYN_BOOTINFO_OFFSET_OF(type, field) __builtin_offsetof(type, field)

#define ORYN_BOOTINFO_STATIC_ASSERT(name, condition) typedef char name[(condition) ? 1 : -1]

#define ORYN_BOOTINFO_FLAG_MEMORY_MAP 0x0000000000000001ULL
#define ORYN_BOOTINFO_FLAG_FRAMEBUFFER 0x0000000000000002ULL
#define ORYN_BOOTINFO_FLAG_RSDP 0x0000000000000004ULL
#define ORYN_BOOTINFO_FLAG_KERNEL_RANGE 0x0000000000000008ULL
#define ORYN_BOOTINFO_FLAG_FONT 0x0000000000000010ULL
#define ORYN_BOOTINFO_FLAG_FIRMWARE_DATA 0x0000000000000020ULL
#define ORYN_BOOTINFO_FLAG_CONFIGURATION_TABLES 0x0000000000000040ULL
#define ORYN_BOOTINFO_FLAG_NVRAM_SNAPSHOT 0x0000000000000080ULL
#define ORYN_BOOTINFO_FLAG_RUNTIME_SERVICES 0x0000000000000100ULL

#define ORYN_BOOTINFO_KNOWN_FLAGS \
    (ORYN_BOOTINFO_FLAG_MEMORY_MAP | \
     ORYN_BOOTINFO_FLAG_FRAMEBUFFER | \
     ORYN_BOOTINFO_FLAG_RSDP | \
     ORYN_BOOTINFO_FLAG_KERNEL_RANGE | \
     ORYN_BOOTINFO_FLAG_FONT | \
     ORYN_BOOTINFO_FLAG_FIRMWARE_DATA | \
     ORYN_BOOTINFO_FLAG_CONFIGURATION_TABLES | \
     ORYN_BOOTINFO_FLAG_NVRAM_SNAPSHOT | \
     ORYN_BOOTINFO_FLAG_RUNTIME_SERVICES)

#define ORYN_BOOTINFO_MAX_CONFIGURATION_TABLES 32U
#define ORYN_BOOTINFO_MAX_BOOT_ORDER_ENTRIES 32U

#define ORYN_FRAMEBUFFER_PIXEL_FORMAT_UNKNOWN 0U
#define ORYN_FRAMEBUFFER_PIXEL_FORMAT_RGBX8888 1U
#define ORYN_FRAMEBUFFER_PIXEL_FORMAT_BGRX8888 2U
#define ORYN_FRAMEBUFFER_PIXEL_FORMAT_BITMASK32 3U

#define ORYN_BOOT_CONFIGURATION_TABLE_UNKNOWN 0U
#define ORYN_BOOT_CONFIGURATION_TABLE_ACPI_10 1U
#define ORYN_BOOT_CONFIGURATION_TABLE_ACPI_20 2U
#define ORYN_BOOT_CONFIGURATION_TABLE_SMBIOS_32 3U
#define ORYN_BOOT_CONFIGURATION_TABLE_SMBIOS_64 4U
#define ORYN_BOOT_CONFIGURATION_TABLE_FDT 5U

#define ORYN_BOOT_NVRAM_FLAG_GET_VARIABLE_AVAILABLE 0x00000001U
#define ORYN_BOOT_NVRAM_FLAG_SECURE_BOOT_PRESENT 0x00000002U
#define ORYN_BOOT_NVRAM_FLAG_SETUP_MODE_PRESENT 0x00000004U
#define ORYN_BOOT_NVRAM_FLAG_AUDIT_MODE_PRESENT 0x00000008U
#define ORYN_BOOT_NVRAM_FLAG_DEPLOYED_MODE_PRESENT 0x00000010U
#define ORYN_BOOT_NVRAM_FLAG_BOOT_ORDER_PRESENT 0x00000020U
#define ORYN_BOOT_NVRAM_FLAG_PK_PRESENT 0x00000040U
#define ORYN_BOOT_NVRAM_FLAG_KEK_PRESENT 0x00000080U
#define ORYN_BOOT_NVRAM_FLAG_DB_PRESENT 0x00000100U
#define ORYN_BOOT_NVRAM_FLAG_DBX_PRESENT 0x00000200U

#define ORYN_BOOT_RUNTIME_FLAG_GET_TIME 0x0000000000000001ULL
#define ORYN_BOOT_RUNTIME_FLAG_SET_TIME 0x0000000000000002ULL
#define ORYN_BOOT_RUNTIME_FLAG_GET_VARIABLE 0x0000000000000004ULL
#define ORYN_BOOT_RUNTIME_FLAG_SET_VARIABLE 0x0000000000000008ULL
#define ORYN_BOOT_RUNTIME_FLAG_RESET_SYSTEM 0x0000000000000010ULL
#define ORYN_BOOT_RUNTIME_FLAG_SET_VIRTUAL_ADDRESS_MAP 0x0000000000000020ULL
#define ORYN_BOOT_RUNTIME_FLAG_CONVERT_POINTER 0x0000000000000040ULL
#define ORYN_BOOT_RUNTIME_FLAG_QUERY_VARIABLE_INFO 0x0000000000000080ULL

typedef struct OrynBootGuid
{
    unsigned int Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} OrynBootGuid;

typedef struct OrynBootMemoryEntry
{
    unsigned int Type;
    unsigned int Reserved;
    unsigned long long PhysicalStart;
    unsigned long long VirtualStart;
    unsigned long long PageCount;
    unsigned long long Attribute;
} OrynBootMemoryEntry;

typedef struct OrynBootConfigurationTableEntry
{
    OrynBootGuid Guid;
    unsigned int Type;
    unsigned int Reserved;
    unsigned long long PhysicalAddress;
} OrynBootConfigurationTableEntry;

typedef struct OrynBootFramebuffer
{
    unsigned long long Base;
    unsigned long long Size;
    unsigned int Width;
    unsigned int Height;
    unsigned int PixelsPerScanLine;
    unsigned int BytesPerPixel;
    unsigned int PixelFormat;
    unsigned int Reserved0;
    unsigned int RedMask;
    unsigned int GreenMask;
    unsigned int BlueMask;
    unsigned int ReservedMask;
} OrynBootFramebuffer;

typedef struct OrynBootFirmwareData
{
    unsigned int Version;
    unsigned int Size;
    unsigned int UefiRevision;
    unsigned int FirmwareRevision;
    unsigned long long ConfigurationTableCount;
    unsigned int BootTimeYear;
    unsigned int BootTimeMonth;
    unsigned int BootTimeDay;
    unsigned int BootTimeHour;
    unsigned int BootTimeMinute;
    unsigned int BootTimeSecond;
    unsigned int BootTimeNanosecond;
    int BootTimeTimeZone;
    unsigned int BootTimeDaylight;
    unsigned int BootTimeValid;
    char FirmwareVendor[64];
} OrynBootFirmwareData;

typedef struct OrynBootNvramSnapshot
{
    unsigned int Version;
    unsigned int Size;
    unsigned int Flags;
    unsigned int SecureBoot;
    unsigned int SetupMode;
    unsigned int AuditMode;
    unsigned int DeployedMode;
    unsigned int BootOrderCount;
    unsigned short BootOrder[ORYN_BOOTINFO_MAX_BOOT_ORDER_ENTRIES];
    unsigned long long PkSize;
    unsigned long long KekSize;
    unsigned long long DbSize;
    unsigned long long DbxSize;
} OrynBootNvramSnapshot;

typedef struct OrynBootRuntimeServices
{
    unsigned int Version;
    unsigned int Size;
    unsigned long long Flags;
    unsigned long long SystemTable;
    unsigned long long RuntimeServicesTable;
    unsigned long long GetTime;
    unsigned long long SetTime;
    unsigned long long GetVariable;
    unsigned long long SetVariable;
    unsigned long long ResetSystem;
    unsigned long long SetVirtualAddressMap;
    unsigned long long ConvertPointer;
    unsigned long long QueryVariableInfo;
} OrynBootRuntimeServices;

typedef struct OrynBootInfo
{
    unsigned long long Signature;
    unsigned int Version;
    unsigned int Size;
    unsigned long long Flags;
    unsigned long long MemoryMap;
    unsigned long long MemoryMapEntryCount;
    unsigned long long MemoryMapEntrySize;
    unsigned int MemoryMapVersion;
    unsigned int Reserved0;
    OrynBootFramebuffer Framebuffer;
    unsigned long long Rsdp;
    unsigned long long Acpi10Rsdp;
    unsigned long long Acpi20Rsdp;
    unsigned long long Smbios32;
    unsigned long long Smbios64;
    unsigned long long FlattenedDeviceTree;
    unsigned int ConfigurationTableEntryCount;
    unsigned int ConfigurationTableEntrySize;
    OrynBootConfigurationTableEntry ConfigurationTables[ORYN_BOOTINFO_MAX_CONFIGURATION_TABLES];
    unsigned long long KernelPhysicalBase;
    unsigned long long KernelSize;
    unsigned long long FontBase;
    unsigned long long FontSize;
    OrynBootFirmwareData FirmwareData;
    OrynBootNvramSnapshot Nvram;
    OrynBootRuntimeServices RuntimeServices;
    char FontName[32];
    char BootLoaderName[32];
    char KernelName[32];
} OrynBootInfo;

static inline int OrynBootInfoAbiIsVersionCompatible(unsigned int version)
{
    return version >= ORYN_BOOTINFO_ABI_MIN_COMPATIBLE_VERSION &&
        version <= ORYN_BOOTINFO_ABI_CURRENT_VERSION;
}

static inline int OrynBootInfoAbiIsSizeCompatible(unsigned int size)
{
    return size >= ORYN_BOOTINFO_ABI_MIN_COMPATIBLE_SIZE;
}

static inline int OrynBootInfoAbiFlagsCompatible(unsigned long long flags)
{
    return (flags & ~ORYN_BOOTINFO_KNOWN_FLAGS) == 0ULL;
}

static inline int OrynBootInfoAbiIsCompatible(const OrynBootInfo* bootInfo)
{
    if (bootInfo == 0)
    {
        return 0;
    }

    if (bootInfo->Signature != ORYN_BOOTINFO_SIGNATURE)
    {
        return 0;
    }

    if (!OrynBootInfoAbiIsVersionCompatible(bootInfo->Version))
    {
        return 0;
    }

    if (!OrynBootInfoAbiIsSizeCompatible(bootInfo->Size))
    {
        return 0;
    }

    if (!OrynBootInfoAbiFlagsCompatible(bootInfo->Flags))
    {
        return 0;
    }

    return 1;
}

ORYN_BOOTINFO_STATIC_ASSERT(OrynBootGuid_must_be_plain_16_bytes, sizeof(OrynBootGuid) == 16U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootMemoryEntry_must_be_plain_40_bytes, sizeof(OrynBootMemoryEntry) == 40U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootFramebuffer_must_be_plain_56_bytes, sizeof(OrynBootFramebuffer) == 56U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootInfo_signature_must_be_64_bit, sizeof(((OrynBootInfo*)0)->Signature) == 8U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootInfo_flags_must_be_64_bit, sizeof(((OrynBootInfo*)0)->Flags) == 8U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootInfo_pointer_fields_are_plain_addresses, sizeof(((OrynBootInfo*)0)->MemoryMap) == 8U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootInfo_framebuffer_base_is_plain_address, sizeof(((OrynBootInfo*)0)->Framebuffer.Base) == 8U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootFirmwareData_stable_size, sizeof(OrynBootFirmwareData) == 128U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootNvramSnapshot_stable_size, sizeof(OrynBootNvramSnapshot) == 128U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootRuntimeServices_stable_size, sizeof(OrynBootRuntimeServices) == 96U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootConfigurationTableEntry_stable_size, sizeof(OrynBootConfigurationTableEntry) == 32U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootInfo_stable_size, sizeof(OrynBootInfo) == ORYN_BOOTINFO_ABI_CURRENT_SIZE);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootInfo_signature_offset_stable, ORYN_BOOTINFO_OFFSET_OF(OrynBootInfo, Signature) == 0U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootInfo_version_offset_stable, ORYN_BOOTINFO_OFFSET_OF(OrynBootInfo, Version) == 8U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootInfo_size_offset_stable, ORYN_BOOTINFO_OFFSET_OF(OrynBootInfo, Size) == 12U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootInfo_flags_offset_stable, ORYN_BOOTINFO_OFFSET_OF(OrynBootInfo, Flags) == 16U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootInfo_memory_map_offset_stable, ORYN_BOOTINFO_OFFSET_OF(OrynBootInfo, MemoryMap) == 24U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootInfo_framebuffer_offset_stable, ORYN_BOOTINFO_OFFSET_OF(OrynBootInfo, Framebuffer) == 56U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootInfo_rsdp_offset_stable, ORYN_BOOTINFO_OFFSET_OF(OrynBootInfo, Rsdp) == 112U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootInfo_configuration_tables_offset_stable, ORYN_BOOTINFO_OFFSET_OF(OrynBootInfo, ConfigurationTables) == 168U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootInfo_kernel_range_offset_stable, ORYN_BOOTINFO_OFFSET_OF(OrynBootInfo, KernelPhysicalBase) == 1192U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootInfo_firmware_data_offset_stable, ORYN_BOOTINFO_OFFSET_OF(OrynBootInfo, FirmwareData) == 1224U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootInfo_nvram_offset_stable, ORYN_BOOTINFO_OFFSET_OF(OrynBootInfo, Nvram) == 1352U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootInfo_runtime_services_offset_stable, ORYN_BOOTINFO_OFFSET_OF(OrynBootInfo, RuntimeServices) == 1480U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootInfo_font_name_offset_stable, ORYN_BOOTINFO_OFFSET_OF(OrynBootInfo, FontName) == 1576U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootInfo_boot_loader_name_offset_stable, ORYN_BOOTINFO_OFFSET_OF(OrynBootInfo, BootLoaderName) == 1608U);
ORYN_BOOTINFO_STATIC_ASSERT(OrynBootInfo_kernel_name_offset_stable, ORYN_BOOTINFO_OFFSET_OF(OrynBootInfo, KernelName) == 1640U);

#endif
