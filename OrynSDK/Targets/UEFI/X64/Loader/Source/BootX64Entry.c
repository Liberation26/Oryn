#include "BootX64Internal.h"

EFI_STATUS efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE* systemTable)
{
    return OrynUefiLoaderMain(imageHandle, systemTable);
}
