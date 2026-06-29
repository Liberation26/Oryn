#ifndef ORYN_UEFI_MINIMAL_H
#define ORYN_UEFI_MINIMAL_H

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
typedef unsigned long long UINTN;
typedef long long INT64;
typedef short INT16;
typedef unsigned short CHAR16;
typedef void* EFI_HANDLE;
typedef UINT64 EFI_STATUS;
typedef UINT64 EFI_PHYSICAL_ADDRESS;
typedef UINT64 EFI_VIRTUAL_ADDRESS;
typedef void VOID;

#define EFI_SUCCESS 0ULL
#define EFI_ERROR_BIT 0x8000000000000000ULL
#define EFI_LOAD_ERROR (EFI_ERROR_BIT | 1ULL)
#define EFI_INVALID_PARAMETER (EFI_ERROR_BIT | 2ULL)
#define EFI_UNSUPPORTED (EFI_ERROR_BIT | 3ULL)
#define EFI_BAD_BUFFER_SIZE (EFI_ERROR_BIT | 4ULL)
#define EFI_BUFFER_TOO_SMALL (EFI_ERROR_BIT | 5ULL)
#define EFI_NOT_READY (EFI_ERROR_BIT | 6ULL)
#define EFI_DEVICE_ERROR (EFI_ERROR_BIT | 7ULL)
#define EFI_NOT_FOUND (EFI_ERROR_BIT | 14ULL)
#define EFI_ABORTED (EFI_ERROR_BIT | 21ULL)

#define EFI_OPEN_PROTOCOL_GET_PROTOCOL 0x00000002ULL
#define EFI_FILE_MODE_READ 0x0000000000000001ULL
#define ORYN_NULL ((void*)0)

typedef struct EFI_GUID
{
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    UINT8 Data4[8];
} EFI_GUID;

typedef struct EFI_TABLE_HEADER
{
    UINT64 Signature;
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 CRC32;
    UINT32 Reserved;
} EFI_TABLE_HEADER;

typedef enum EFI_ALLOCATE_TYPE
{
    AllocateAnyPages = 0,
    AllocateMaxAddress = 1,
    AllocateAddress = 2
} EFI_ALLOCATE_TYPE;

typedef enum EFI_MEMORY_TYPE
{
    EfiReservedMemoryType = 0,
    EfiLoaderCode = 1,
    EfiLoaderData = 2,
    EfiBootServicesCode = 3,
    EfiBootServicesData = 4,
    EfiRuntimeServicesCode = 5,
    EfiRuntimeServicesData = 6,
    EfiConventionalMemory = 7,
    EfiUnusableMemory = 8,
    EfiACPIReclaimMemory = 9,
    EfiACPIMemoryNVS = 10,
    EfiMemoryMappedIO = 11,
    EfiMemoryMappedIOPortSpace = 12,
    EfiPalCode = 13,
    EfiPersistentMemory = 14
} EFI_MEMORY_TYPE;

typedef struct EFI_MEMORY_DESCRIPTOR
{
    UINT32 Type;
    UINT32 Pad;
    EFI_PHYSICAL_ADDRESS PhysicalStart;
    EFI_VIRTUAL_ADDRESS VirtualStart;
    UINT64 NumberOfPages;
    UINT64 Attribute;
} EFI_MEMORY_DESCRIPTOR;


typedef struct EFI_TIME
{
    UINT16 Year;
    UINT8 Month;
    UINT8 Day;
    UINT8 Hour;
    UINT8 Minute;
    UINT8 Second;
    UINT8 Pad1;
    UINT32 Nanosecond;
    INT16 TimeZone;
    UINT8 Daylight;
    UINT8 Pad2;
} EFI_TIME;

typedef struct EFI_TIME_CAPABILITIES
{
    UINT32 Resolution;
    UINT32 Accuracy;
    UINT8 SetsToZero;
} EFI_TIME_CAPABILITIES;

typedef EFI_STATUS (*EFI_GET_TIME)(EFI_TIME*, EFI_TIME_CAPABILITIES*);
typedef EFI_STATUS (*EFI_SET_TIME)(EFI_TIME*);
typedef EFI_STATUS (*EFI_GET_VARIABLE)(CHAR16*, EFI_GUID*, UINT32*, UINTN*, void*);
typedef EFI_STATUS (*EFI_SET_VARIABLE)(CHAR16*, EFI_GUID*, UINT32, UINTN, void*);

typedef struct EFI_RUNTIME_SERVICES
{
    EFI_TABLE_HEADER Hdr;
    EFI_GET_TIME GetTime;
    EFI_SET_TIME SetTime;
    void* GetWakeupTime;
    void* SetWakeupTime;
    void* SetVirtualAddressMap;
    void* ConvertPointer;
    EFI_GET_VARIABLE GetVariable;
    void* GetNextVariableName;
    EFI_SET_VARIABLE SetVariable;
    void* GetNextHighMonotonicCount;
    void* ResetSystem;
    void* UpdateCapsule;
    void* QueryCapsuleCapabilities;
    void* QueryVariableInfo;
} EFI_RUNTIME_SERVICES;

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
typedef EFI_STATUS (*EFI_TEXT_RESET)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*, unsigned char);
typedef EFI_STATUS (*EFI_TEXT_STRING)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*, CHAR16*);

struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
{
    EFI_TEXT_RESET Reset;
    EFI_TEXT_STRING OutputString;
    void* TestString;
    void* QueryMode;
    void* SetMode;
    void* SetAttribute;
    void* ClearScreen;
    void* SetCursorPosition;
    void* EnableCursor;
    void* Mode;
};

typedef struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;
typedef struct EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;

typedef EFI_STATUS (*EFI_OPEN_VOLUME)(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*, EFI_FILE_PROTOCOL**);
struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
{
    UINT64 Revision;
    EFI_OPEN_VOLUME OpenVolume;
};

typedef EFI_STATUS (*EFI_FILE_OPEN)(EFI_FILE_PROTOCOL*, EFI_FILE_PROTOCOL**, CHAR16*, UINT64, UINT64);
typedef EFI_STATUS (*EFI_FILE_CLOSE)(EFI_FILE_PROTOCOL*);
typedef EFI_STATUS (*EFI_FILE_DELETE)(EFI_FILE_PROTOCOL*);
typedef EFI_STATUS (*EFI_FILE_READ)(EFI_FILE_PROTOCOL*, UINTN*, void*);
typedef EFI_STATUS (*EFI_FILE_WRITE)(EFI_FILE_PROTOCOL*, UINTN*, void*);

struct EFI_FILE_PROTOCOL
{
    UINT64 Revision;
    EFI_FILE_OPEN Open;
    EFI_FILE_CLOSE Close;
    EFI_FILE_DELETE Delete;
    EFI_FILE_READ Read;
    EFI_FILE_WRITE Write;
    void* GetPosition;
    void* SetPosition;
    void* GetInfo;
    void* SetInfo;
    void* Flush;
    void* OpenEx;
    void* ReadEx;
    void* WriteEx;
    void* FlushEx;
};

typedef struct EFI_LOADED_IMAGE_PROTOCOL
{
    UINT32 Revision;
    EFI_HANDLE ParentHandle;
    void* SystemTable;
    EFI_HANDLE DeviceHandle;
    void* FilePath;
    void* Reserved;
    UINT32 LoadOptionsSize;
    void* LoadOptions;
    void* ImageBase;
    UINT64 ImageSize;
    EFI_MEMORY_TYPE ImageCodeType;
    EFI_MEMORY_TYPE ImageDataType;
    EFI_STATUS (*Unload)(EFI_HANDLE ImageHandle);
} EFI_LOADED_IMAGE_PROTOCOL;

typedef struct EFI_BOOT_SERVICES
{
    EFI_TABLE_HEADER Hdr;
    void* RaiseTPL;
    void* RestoreTPL;
    EFI_STATUS (*AllocatePages)(EFI_ALLOCATE_TYPE, EFI_MEMORY_TYPE, UINTN, EFI_PHYSICAL_ADDRESS*);
    EFI_STATUS (*FreePages)(EFI_PHYSICAL_ADDRESS, UINTN);
    EFI_STATUS (*GetMemoryMap)(UINTN*, EFI_MEMORY_DESCRIPTOR*, UINTN*, UINTN*, UINT32*);
    EFI_STATUS (*AllocatePool)(EFI_MEMORY_TYPE, UINTN, void**);
    EFI_STATUS (*FreePool)(void*);
    void* CreateEvent;
    void* SetTimer;
    void* WaitForEvent;
    void* SignalEvent;
    void* CloseEvent;
    void* CheckEvent;
    void* InstallProtocolInterface;
    void* ReinstallProtocolInterface;
    void* UninstallProtocolInterface;
    void* HandleProtocol;
    void* Reserved;
    void* RegisterProtocolNotify;
    void* LocateHandle;
    void* LocateDevicePath;
    void* InstallConfigurationTable;
    void* LoadImage;
    void* StartImage;
    void* Exit;
    void* UnloadImage;
    EFI_STATUS (*ExitBootServices)(EFI_HANDLE, UINTN);
    void* GetNextMonotonicCount;
    EFI_STATUS (*Stall)(UINTN);
    EFI_STATUS (*SetWatchdogTimer)(UINTN, UINT64, UINTN, CHAR16*);
    void* ConnectController;
    void* DisconnectController;
    EFI_STATUS (*OpenProtocol)(EFI_HANDLE, EFI_GUID*, void**, EFI_HANDLE, EFI_HANDLE, UINT32);
    EFI_STATUS (*CloseProtocol)(EFI_HANDLE, EFI_GUID*, EFI_HANDLE, EFI_HANDLE);
    void* OpenProtocolInformation;
    void* ProtocolsPerHandle;
    void* LocateHandleBuffer;
    EFI_STATUS (*LocateProtocol)(EFI_GUID*, void*, void**);
    void* InstallMultipleProtocolInterfaces;
    void* UninstallMultipleProtocolInterfaces;
    void* CalculateCrc32;
    void* CopyMem;
    void* SetMem;
    void* CreateEventEx;
} EFI_BOOT_SERVICES;


typedef struct EFI_CONFIGURATION_TABLE
{
    EFI_GUID VendorGuid;
    void* VendorTable;
} EFI_CONFIGURATION_TABLE;

typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE_INFORMATION
{
    UINT32 Version;
    UINT32 HorizontalResolution;
    UINT32 VerticalResolution;
    UINT32 PixelFormat;
    UINT32 PixelInformation[4];
    UINT32 PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE_INFORMATION;

typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE
{
    UINT32 MaxMode;
    UINT32 Mode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE_INFORMATION* Info;
    UINTN SizeOfInfo;
    EFI_PHYSICAL_ADDRESS FrameBufferBase;
    UINTN FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

struct EFI_GRAPHICS_OUTPUT_PROTOCOL
{
    void* QueryMode;
    void* SetMode;
    void* Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE* Mode;
};

typedef struct EFI_SYSTEM_TABLE
{
    EFI_TABLE_HEADER Hdr;
    CHAR16* FirmwareVendor;
    UINT32 FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    void* ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut;
    EFI_HANDLE StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* StdErr;
    EFI_RUNTIME_SERVICES* RuntimeServices;
    EFI_BOOT_SERVICES* BootServices;
    UINTN NumberOfTableEntries;
    EFI_CONFIGURATION_TABLE* ConfigurationTable;
} EFI_SYSTEM_TABLE;

#endif
