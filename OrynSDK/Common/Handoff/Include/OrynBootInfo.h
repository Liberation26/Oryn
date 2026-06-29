#ifndef ORYN_BOOT_INFO_H
#define ORYN_BOOT_INFO_H

#define ORYN_BOOTINFO_SIGNATURE 0x544F4F424E59524FULL
#define ORYN_BOOTINFO_VERSION 3U

#define ORYN_BOOTINFO_FLAG_MEMORY_MAP 0x0000000000000001ULL
#define ORYN_BOOTINFO_FLAG_FRAMEBUFFER 0x0000000000000002ULL
#define ORYN_BOOTINFO_FLAG_RSDP 0x0000000000000004ULL
#define ORYN_BOOTINFO_FLAG_KERNEL_RANGE 0x0000000000000008ULL
#define ORYN_BOOTINFO_FLAG_FONT 0x0000000000000010ULL
#define ORYN_BOOTINFO_FLAG_FIRMWARE_DATA 0x0000000000000020ULL
#define ORYN_BOOTINFO_FLAG_CONFIGURATION_TABLES 0x0000000000000040ULL
#define ORYN_BOOTINFO_FLAG_NVRAM_SNAPSHOT 0x0000000000000080ULL
#define ORYN_BOOTINFO_FLAG_RUNTIME_SERVICES 0x0000000000000100ULL

#define ORYN_BOOTINFO_MAX_CONFIGURATION_TABLES 32U
#define ORYN_BOOTINFO_MAX_BOOT_ORDER_ENTRIES 32U

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
    unsigned long long FramebufferBase;
    unsigned long long FramebufferSize;
    unsigned int FramebufferWidth;
    unsigned int FramebufferHeight;
    unsigned int FramebufferPixelsPerScanLine;
    unsigned int FramebufferPixelFormat;
    unsigned int FramebufferMode;
    unsigned int FramebufferMaxMode;
    unsigned int FramebufferInfoVersion;
    unsigned int FramebufferInfoSize;
    unsigned int FramebufferRedMask;
    unsigned int FramebufferGreenMask;
    unsigned int FramebufferBlueMask;
    unsigned int FramebufferReservedMask;
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

#endif
