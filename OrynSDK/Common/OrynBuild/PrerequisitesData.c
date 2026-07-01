#include "OrynBuild.h"
#include <stdio.h>
#include <string.h>

typedef struct OrynPrerequisite
{
    const char* tool;
    const char* purpose;
    const char* apt_package;
    const char* dnf_package;
    const char* pacman_package;
    const char* zypper_package;
    const char* brew_package;
    const char* winget_package;
    int required;
} OrynPrerequisite;

const OrynPrerequisite* OrynPrereqTable(int* count)
{
    static const OrynPrerequisite table[] =
    {
        { "clang", "C compiler for native SDK and kernel units", "clang", "clang", "clang", "clang", "llvm", "LLVM.LLVM", 1 },
        { "ld.lld", "ELF linker for freestanding kernels", "lld", "lld", "lld", "lld", "llvm", "LLVM.LLVM", 1 },
        { "llvm-ar", "Static archive creation for module libraries", "llvm", "llvm", "llvm", "llvm", "llvm", "LLVM.LLVM", 1 },
        { "llvm-objcopy", "ELF and binary post-processing", "llvm", "llvm", "llvm", "llvm", "llvm", "LLVM.LLVM", 1 },
        { "git", "source sync and update workflows", "git", "git", "git", "git", "git", "Git.Git", 1 },
        { "qemu-system-x86_64", "local VM execution", "qemu-system-x86", "qemu-system-x86", "qemu-system-x86", "qemu-x86", "qemu", "SoftwareFreedomConservancy.QEMU", 1 },
        { "mkfs.vfat", "FAT32 ESP formatting", "dosfstools", "dosfstools", "dosfstools", "dosfstools", "dosfstools", "", 1 },
        { "mcopy", "FAT image file population", "mtools", "mtools", "mtools", "mtools", "mtools", "", 1 },
        { "zip", "SDK package creation", "zip", "zip", "zip", "zip", "zip", "GnuWin32.Zip", 1 },
        { "unzip", "SDK package extraction", "unzip", "unzip", "unzip", "unzip", "unzip", "GnuWin32.UnZip", 1 }
    };
    *count = (int)(sizeof(table) / sizeof(table[0]));
    return table;
}

const char* OrynPrereqPackageForProvider(const OrynPrerequisite* item, const char* provider)
{
    if (strcmp(provider, "apt") == 0) return item->apt_package;
    if (strcmp(provider, "dnf") == 0) return item->dnf_package;
    if (strcmp(provider, "pacman") == 0) return item->pacman_package;
    if (strcmp(provider, "zypper") == 0) return item->zypper_package;
    if (strcmp(provider, "brew") == 0) return item->brew_package;
    if (strcmp(provider, "winget") == 0) return item->winget_package;
    return "";
}
