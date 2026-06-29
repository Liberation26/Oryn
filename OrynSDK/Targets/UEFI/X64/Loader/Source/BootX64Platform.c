#include "BootX64Internal.h"

static EFI_GUID gAcpi20TableGuid =
{
    0x8868E871U, 0xE4F1U, 0x11D3U,
    { 0xBC, 0x22, 0x00, 0x80, 0xC7, 0x3C, 0x88, 0x81 }
};

static EFI_GUID gAcpi10TableGuid =
{
    0xEB9D2D30U, 0x2D88U, 0x11D3U,
    { 0x9A, 0x16, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D }
};

static EFI_GUID gSmbiosTableGuid =
{
    0xEB9D2D31U, 0x2D88U, 0x11D3U,
    { 0x9A, 0x16, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D }
};

static EFI_GUID gSmbios3TableGuid =
{
    0xF2FD1544U, 0x9794U, 0x4A2CU,
    { 0x99, 0x2E, 0xE5, 0xBB, 0xCF, 0x20, 0xE3, 0x94 }
};

static EFI_GUID gFdtTableGuid =
{
    0xB1B621D5U, 0xF19CU, 0x41A5U,
    { 0x83, 0x0B, 0xD9, 0x15, 0x2C, 0x69, 0xAA, 0xE0 }
};

static EFI_GUID gEfiGlobalVariableGuid =
{
    0x8BE4DF61U, 0x93CAU, 0x11D2U,
    { 0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C }
};

static EFI_GUID gImageSecurityDatabaseGuid =
{
    0xD719B2CBU, 0x3D3AU, 0x4596U,
    { 0xA3, 0xBC, 0xDA, 0xD0, 0x0E, 0x67, 0x65, 0x6F }
};

static CHAR16 gSecureBootName[] = { 'S', 'e', 'c', 'u', 'r', 'e', 'B', 'o', 'o', 't', 0 };
static CHAR16 gSetupModeName[] = { 'S', 'e', 't', 'u', 'p', 'M', 'o', 'd', 'e', 0 };
static CHAR16 gAuditModeName[] = { 'A', 'u', 'd', 'i', 't', 'M', 'o', 'd', 'e', 0 };
static CHAR16 gDeployedModeName[] = { 'D', 'e', 'p', 'l', 'o', 'y', 'e', 'd', 'M', 'o', 'd', 'e', 0 };
static CHAR16 gBootOrderName[] = { 'B', 'o', 'o', 't', 'O', 'r', 'd', 'e', 'r', 0 };
static CHAR16 gPkName[] = { 'P', 'K', 0 };
static CHAR16 gKekName[] = { 'K', 'E', 'K', 0 };
static CHAR16 gDbName[] = { 'd', 'b', 0 };
static CHAR16 gDbxName[] = { 'd', 'b', 'x', 0 };

static int GuidEquals(const EFI_GUID* left, const EFI_GUID* right)
{
    if (left->Data1 != right->Data1 || left->Data2 != right->Data2 || left->Data3 != right->Data3)
    {
        return 0;
    }

    for (int index = 0; index < 8; ++index)
    {
        if (left->Data4[index] != right->Data4[index])
        {
            return 0;
        }
    }

    return 1;
}

static void CopyGuid(OrynBootGuid* target, const EFI_GUID* source)
{
    target->Data1 = source->Data1;
    target->Data2 = source->Data2;
    target->Data3 = source->Data3;
    for (int index = 0; index < 8; ++index)
    {
        target->Data4[index] = source->Data4[index];
    }
}

static unsigned int ClassifyConfigurationTable(const EFI_GUID* guid)
{
    if (GuidEquals(guid, &gAcpi20TableGuid))
    {
        return ORYN_BOOT_CONFIGURATION_TABLE_ACPI_20;
    }

    if (GuidEquals(guid, &gAcpi10TableGuid))
    {
        return ORYN_BOOT_CONFIGURATION_TABLE_ACPI_10;
    }

    if (GuidEquals(guid, &gSmbiosTableGuid))
    {
        return ORYN_BOOT_CONFIGURATION_TABLE_SMBIOS_32;
    }

    if (GuidEquals(guid, &gSmbios3TableGuid))
    {
        return ORYN_BOOT_CONFIGURATION_TABLE_SMBIOS_64;
    }

    if (GuidEquals(guid, &gFdtTableGuid))
    {
        return ORYN_BOOT_CONFIGURATION_TABLE_FDT;
    }

    return ORYN_BOOT_CONFIGURATION_TABLE_UNKNOWN;
}

static void StoreDirectPlatformPointer(OrynBootInfo* bootInfo, unsigned int type, UINT64 address)
{
    if (type == ORYN_BOOT_CONFIGURATION_TABLE_ACPI_20)
    {
        bootInfo->Acpi20Rsdp = address;
        bootInfo->Rsdp = address;
    }
    else if (type == ORYN_BOOT_CONFIGURATION_TABLE_ACPI_10)
    {
        bootInfo->Acpi10Rsdp = address;
        if (bootInfo->Rsdp == 0ULL)
        {
            bootInfo->Rsdp = address;
        }
    }
    else if (type == ORYN_BOOT_CONFIGURATION_TABLE_SMBIOS_32)
    {
        bootInfo->Smbios32 = address;
    }
    else if (type == ORYN_BOOT_CONFIGURATION_TABLE_SMBIOS_64)
    {
        bootInfo->Smbios64 = address;
    }
    else if (type == ORYN_BOOT_CONFIGURATION_TABLE_FDT)
    {
        bootInfo->FlattenedDeviceTree = address;
    }
}

static void StoreConfigurationTable(OrynBootInfo* bootInfo, EFI_CONFIGURATION_TABLE* table)
{
    unsigned int index = bootInfo->ConfigurationTableEntryCount;
    if (index >= ORYN_BOOTINFO_MAX_CONFIGURATION_TABLES)
    {
        return;
    }

    OrynBootConfigurationTableEntry* target = &bootInfo->ConfigurationTables[index];
    unsigned int type = ClassifyConfigurationTable(&table->VendorGuid);
    CopyGuid(&target->Guid, &table->VendorGuid);
    target->Type = type;
    target->Reserved = 0U;
    target->PhysicalAddress = (UINT64)(UINTN)table->VendorTable;
    bootInfo->ConfigurationTableEntryCount = index + 1U;
    StoreDirectPlatformPointer(bootInfo, type, target->PhysicalAddress);
}

void OrynCapturePlatformTables(OrynBootInfo* bootInfo, int wantPlatformTables, int wantRsdp)
{
    bootInfo->ConfigurationTableEntryCount = 0U;
    bootInfo->ConfigurationTableEntrySize = sizeof(OrynBootConfigurationTableEntry);
    bootInfo->Rsdp = 0ULL;
    bootInfo->Acpi10Rsdp = 0ULL;
    bootInfo->Acpi20Rsdp = 0ULL;
    bootInfo->Smbios32 = 0ULL;
    bootInfo->Smbios64 = 0ULL;
    bootInfo->FlattenedDeviceTree = 0ULL;

    if (gSystemTable->ConfigurationTable == ORYN_NULL || gSystemTable->NumberOfTableEntries == 0)
    {
        Print("[BOOT] BootInfo configuration tables: none exposed by firmware.\n");
        return;
    }

    for (UINTN index = 0; index < gSystemTable->NumberOfTableEntries; ++index)
    {
        StoreConfigurationTable(bootInfo, &gSystemTable->ConfigurationTable[index]);
    }

    if (wantPlatformTables && bootInfo->ConfigurationTableEntryCount > 0U)
    {
        bootInfo->Flags |= ORYN_BOOTINFO_FLAG_CONFIGURATION_TABLES;
    }

    if (wantRsdp && bootInfo->Rsdp != 0ULL)
    {
        bootInfo->Flags |= ORYN_BOOTINFO_FLAG_RSDP;
    }

    Print("[BOOT] BootInfo configuration table entries copied: ");
    PrintHex64(bootInfo->ConfigurationTableEntryCount);
    Print("\n");
    Print("[BOOT] BootInfo ACPI RSDP: ");
    PrintHex64(bootInfo->Rsdp);
    Print("\n");
    Print("[BOOT] BootInfo SMBIOS32: ");
    PrintHex64(bootInfo->Smbios32);
    Print("\n");
    Print("[BOOT] BootInfo SMBIOS64: ");
    PrintHex64(bootInfo->Smbios64);
    Print("\n");
    Print("[BOOT] BootInfo FDT: ");
    PrintHex64(bootInfo->FlattenedDeviceTree);
    Print("\n");
}

static int ReadByteVariable(CHAR16* name, EFI_GUID* guid, unsigned int* value)
{
    UINT8 byteValue = 0;
    UINTN size = sizeof(byteValue);
    UINT32 attributes = 0U;
    EFI_STATUS status;

    if (gSystemTable->RuntimeServices == ORYN_NULL ||
        gSystemTable->RuntimeServices->GetVariable == ORYN_NULL)
    {
        return 0;
    }

    status = gSystemTable->RuntimeServices->GetVariable(name, guid, &attributes, &size, &byteValue);
    if (IsError(status) || size < 1U)
    {
        return 0;
    }

    *value = byteValue;
    return 1;
}

static int QueryVariableSize(CHAR16* name, EFI_GUID* guid, unsigned long long* value)
{
    UINTN size = 0;
    UINT32 attributes = 0U;
    EFI_STATUS status;

    if (gSystemTable->RuntimeServices == ORYN_NULL ||
        gSystemTable->RuntimeServices->GetVariable == ORYN_NULL)
    {
        return 0;
    }

    status = gSystemTable->RuntimeServices->GetVariable(name, guid, &attributes, &size, ORYN_NULL);
    if (status == EFI_BUFFER_TOO_SMALL || status == EFI_SUCCESS)
    {
        *value = (unsigned long long)size;
        return 1;
    }

    return 0;
}

static void CaptureBootOrder(OrynBootNvramSnapshot* nvram)
{
    UINTN size = sizeof(nvram->BootOrder);
    UINT32 attributes = 0U;
    EFI_STATUS status;

    if (gSystemTable->RuntimeServices == ORYN_NULL ||
        gSystemTable->RuntimeServices->GetVariable == ORYN_NULL)
    {
        return;
    }

    status = gSystemTable->RuntimeServices->GetVariable(
        gBootOrderName,
        &gEfiGlobalVariableGuid,
        &attributes,
        &size,
        nvram->BootOrder);

    if (!IsError(status) && size >= sizeof(UINT16))
    {
        nvram->BootOrderCount = (unsigned int)(size / sizeof(UINT16));
        if (nvram->BootOrderCount > ORYN_BOOTINFO_MAX_BOOT_ORDER_ENTRIES)
        {
            nvram->BootOrderCount = ORYN_BOOTINFO_MAX_BOOT_ORDER_ENTRIES;
        }
        nvram->Flags |= ORYN_BOOT_NVRAM_FLAG_BOOT_ORDER_PRESENT;
    }
}

void OrynCaptureNvramSnapshot(OrynBootInfo* bootInfo)
{
    OrynBootNvramSnapshot* nvram = &bootInfo->Nvram;
    SetMemory(nvram, 0, sizeof(*nvram));
    nvram->Version = 1U;
    nvram->Size = sizeof(*nvram);

    if (gSystemTable->RuntimeServices == ORYN_NULL ||
        gSystemTable->RuntimeServices->GetVariable == ORYN_NULL)
    {
        Print("[BOOT] BootInfo NVRAM snapshot: GetVariable unavailable.\n");
        return;
    }

    nvram->Flags |= ORYN_BOOT_NVRAM_FLAG_GET_VARIABLE_AVAILABLE;

    if (ReadByteVariable(gSecureBootName, &gEfiGlobalVariableGuid, &nvram->SecureBoot))
    {
        nvram->Flags |= ORYN_BOOT_NVRAM_FLAG_SECURE_BOOT_PRESENT;
    }

    if (ReadByteVariable(gSetupModeName, &gEfiGlobalVariableGuid, &nvram->SetupMode))
    {
        nvram->Flags |= ORYN_BOOT_NVRAM_FLAG_SETUP_MODE_PRESENT;
    }

    if (ReadByteVariable(gAuditModeName, &gEfiGlobalVariableGuid, &nvram->AuditMode))
    {
        nvram->Flags |= ORYN_BOOT_NVRAM_FLAG_AUDIT_MODE_PRESENT;
    }

    if (ReadByteVariable(gDeployedModeName, &gEfiGlobalVariableGuid, &nvram->DeployedMode))
    {
        nvram->Flags |= ORYN_BOOT_NVRAM_FLAG_DEPLOYED_MODE_PRESENT;
    }

    CaptureBootOrder(nvram);

    if (QueryVariableSize(gPkName, &gEfiGlobalVariableGuid, &nvram->PkSize))
    {
        nvram->Flags |= ORYN_BOOT_NVRAM_FLAG_PK_PRESENT;
    }

    if (QueryVariableSize(gKekName, &gEfiGlobalVariableGuid, &nvram->KekSize))
    {
        nvram->Flags |= ORYN_BOOT_NVRAM_FLAG_KEK_PRESENT;
    }

    if (QueryVariableSize(gDbName, &gImageSecurityDatabaseGuid, &nvram->DbSize))
    {
        nvram->Flags |= ORYN_BOOT_NVRAM_FLAG_DB_PRESENT;
    }

    if (QueryVariableSize(gDbxName, &gImageSecurityDatabaseGuid, &nvram->DbxSize))
    {
        nvram->Flags |= ORYN_BOOT_NVRAM_FLAG_DBX_PRESENT;
    }

    bootInfo->Flags |= ORYN_BOOTINFO_FLAG_NVRAM_SNAPSHOT;
    Print("[BOOT] BootInfo NVRAM snapshot: captured.\n");
    Print("[BOOT] BootInfo SecureBoot value: ");
    PrintHex64(nvram->SecureBoot);
    Print("\n");
    Print("[BOOT] BootInfo BootOrder entries: ");
    PrintHex64(nvram->BootOrderCount);
    Print("\n");
}

static void StoreRuntimePointer(unsigned long long* target, unsigned long long* flags,
    unsigned long long flag, void* pointer)
{
    *target = (UINT64)(UINTN)pointer;
    if (pointer != ORYN_NULL)
    {
        *flags |= flag;
    }
}

void OrynCaptureRuntimeServices(OrynBootInfo* bootInfo)
{
    OrynBootRuntimeServices* runtime = &bootInfo->RuntimeServices;
    SetMemory(runtime, 0, sizeof(*runtime));
    runtime->Version = 1U;
    runtime->Size = sizeof(*runtime);
    runtime->SystemTable = (UINT64)(UINTN)gSystemTable;

    if (gSystemTable->RuntimeServices == ORYN_NULL)
    {
        Print("[BOOT] BootInfo RuntimeServices: unavailable.\n");
        return;
    }

    runtime->RuntimeServicesTable = (UINT64)(UINTN)gSystemTable->RuntimeServices;
    StoreRuntimePointer(&runtime->GetTime, &runtime->Flags,
        ORYN_BOOT_RUNTIME_FLAG_GET_TIME, (void*)gSystemTable->RuntimeServices->GetTime);
    StoreRuntimePointer(&runtime->SetTime, &runtime->Flags,
        ORYN_BOOT_RUNTIME_FLAG_SET_TIME, (void*)gSystemTable->RuntimeServices->SetTime);
    StoreRuntimePointer(&runtime->GetVariable, &runtime->Flags,
        ORYN_BOOT_RUNTIME_FLAG_GET_VARIABLE, (void*)gSystemTable->RuntimeServices->GetVariable);
    StoreRuntimePointer(&runtime->SetVariable, &runtime->Flags,
        ORYN_BOOT_RUNTIME_FLAG_SET_VARIABLE, (void*)gSystemTable->RuntimeServices->SetVariable);
    StoreRuntimePointer(&runtime->ResetSystem, &runtime->Flags,
        ORYN_BOOT_RUNTIME_FLAG_RESET_SYSTEM, gSystemTable->RuntimeServices->ResetSystem);
    StoreRuntimePointer(&runtime->SetVirtualAddressMap, &runtime->Flags,
        ORYN_BOOT_RUNTIME_FLAG_SET_VIRTUAL_ADDRESS_MAP, gSystemTable->RuntimeServices->SetVirtualAddressMap);
    StoreRuntimePointer(&runtime->ConvertPointer, &runtime->Flags,
        ORYN_BOOT_RUNTIME_FLAG_CONVERT_POINTER, gSystemTable->RuntimeServices->ConvertPointer);
    StoreRuntimePointer(&runtime->QueryVariableInfo, &runtime->Flags,
        ORYN_BOOT_RUNTIME_FLAG_QUERY_VARIABLE_INFO, gSystemTable->RuntimeServices->QueryVariableInfo);

    if (runtime->RuntimeServicesTable != 0ULL)
    {
        bootInfo->Flags |= ORYN_BOOTINFO_FLAG_RUNTIME_SERVICES;
    }

    Print("[BOOT] BootInfo RuntimeServices table: ");
    PrintHex64(runtime->RuntimeServicesTable);
    Print("\n");
    Print("[BOOT] BootInfo RuntimeServices flags: ");
    PrintHex64(runtime->Flags);
    Print("\n");
}
