#ifndef ORYN_BOOT_X64_INTERNAL_H
#define ORYN_BOOT_X64_INTERNAL_H

#include "UefiMinimal.h"
#include "OrynBootInfo.h"

#define ELF_MAGIC_0 0x7f
#define ELF_MAGIC_1 'E'
#define ELF_MAGIC_2 'L'
#define ELF_MAGIC_3 'F'
#define ELF_CLASS_64 2
#define ELF_DATA_LITTLE 1
#define ELF_TYPE_EXEC 2
#define ELF_MACHINE_X86_64 0x3e
#define ELF_PT_LOAD 1
#define PAGE_SIZE 4096ULL
#define MAX_KERNEL_FILE_SIZE (16ULL * 1024ULL * 1024ULL)
#define MAX_TTF_FILE_SIZE (8ULL * 1024ULL * 1024ULL)
#define SERIAL_COM1 0x3F8U
#define QEMU_DEBUG_PORT 0xE9U



typedef struct Elf64_Ehdr
{
    UINT8 e_ident[16];
    UINT16 e_type;
    UINT16 e_machine;
    UINT32 e_version;
    UINT64 e_entry;
    UINT64 e_phoff;
    UINT64 e_shoff;
    UINT32 e_flags;
    UINT16 e_ehsize;
    UINT16 e_phentsize;
    UINT16 e_phnum;
    UINT16 e_shentsize;
    UINT16 e_shnum;
    UINT16 e_shstrndx;
} Elf64_Ehdr;

typedef struct Elf64_Phdr
{
    UINT32 p_type;
    UINT32 p_flags;
    UINT64 p_offset;
    UINT64 p_vaddr;
    UINT64 p_paddr;
    UINT64 p_filesz;
    UINT64 p_memsz;
    UINT64 p_align;
} Elf64_Phdr;

extern EFI_SYSTEM_TABLE* gSystemTable;
extern EFI_BOOT_SERVICES* gBootServices;
extern EFI_GUID gLoadedImageProtocolGuid;
extern EFI_GUID gSimpleFileSystemProtocolGuid;

int IsError(EFI_STATUS status);
void InitSerialDebug(void);
void Print(const char* text);
void PrintHex64(UINT64 value);
void SetMemory(void* target, UINT8 value, UINTN size);
void CopyMemory(void* target, const void* source, UINTN size);
EFI_STATUS OpenKernelFile(EFI_HANDLE imageHandle, EFI_FILE_PROTOCOL** outFile);
EFI_STATUS ReadKernelFile(EFI_FILE_PROTOCOL* file, void** outBuffer, UINTN* outSize);
EFI_STATUS ReadOptionalFontFile(EFI_HANDLE imageHandle, void** outBuffer, UINTN* outSize);
EFI_STATUS LoadElfSegments(const void* kernelBuffer, UINTN kernelSize, UINT64* outEntry, UINT64* outKernelBase, UINT64* outKernelSize);
void OrynCapturePlatformTables(OrynBootInfo* bootInfo, int wantPlatformTables, int wantRsdp);
void OrynCaptureNvramSnapshot(OrynBootInfo* bootInfo);
void OrynCaptureRuntimeServices(OrynBootInfo* bootInfo);
EFI_STATUS ExitBootServicesWithBootInfo(EFI_HANDLE imageHandle, OrynBootInfo* bootInfo);
__attribute__((noreturn)) void JumpToKernel(UINT64 kernelEntry, OrynBootInfo* bootInfo);
EFI_STATUS OrynUefiLoaderMain(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE* systemTable);

#endif
