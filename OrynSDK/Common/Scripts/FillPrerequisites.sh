#!/usr/bin/env bash
set -euo pipefail

ColorReset="\033[0m"
ColorOk="\033[32m"
ColorWarn="\033[33m"
ColorInfo="\033[36m"
ColorFail="\033[31m"
UseColor=1
[ -n "${NO_COLOR:-}" ] && UseColor=0
[ "${ORYN_NO_COLOR:-0}" = "1" ] && UseColor=0

PrintTag()
{
    local color="$1"
    local tag="$2"
    local message="$3"

    if [ "$UseColor" -eq 1 ]; then
        printf "%b%s%b %s\n" "$color" "$tag" "$ColorReset" "$message"
    else
        printf "%s %s\n" "$tag" "$message"
    fi
}

Ok() { PrintTag "$ColorOk" "[ OK ]" "$1"; }
Warn() { PrintTag "$ColorWarn" "[WARN]" "$1"; }
Info() { PrintTag "$ColorInfo" "[INFO]" "$1"; }
Fail() { PrintTag "$ColorFail" "[FAIL]" "$1"; }

NeedCommand()
{
    local command_name="$1"
    local package_name="$2"

    if command -v "$command_name" >/dev/null 2>&1; then
        Ok "Found $command_name."
        return 0
    fi

    MISSING_PACKAGES+=("$package_name")
    Warn "Missing $command_name; package $package_name is required."
}

InstallMissingPackages()
{
    if [ "${#MISSING_PACKAGES[@]}" -eq 0 ]; then
        Ok "All apt-managed prerequisites are already present."
        return 0
    fi

    if ! command -v apt-get >/dev/null 2>&1; then
        Fail "apt-get was not found; install manually: ${MISSING_PACKAGES[*]}"
        return 1
    fi

    Info "Installing missing packages: ${MISSING_PACKAGES[*]}"
    sudo apt-get update
    sudo apt-get install -y "${MISSING_PACKAGES[@]}"
}

CheckWindowsQemu()
{
    local qemu_path="/mnt/c/Program Files/qemu/qemu-system-x86_64.exe"
    local ovmf_path="/mnt/c/Program Files/qemu/share/edk2-x86_64-code.fd"

    if [ -x "$qemu_path" ]; then
        Ok "Found Windows QEMU: $qemu_path"
    else
        Warn "Windows QEMU was not found at $qemu_path."
    fi

    if [ -f "$ovmf_path" ]; then
        Ok "Found Windows OVMF: $ovmf_path"
    else
        Warn "Windows OVMF was not found at $ovmf_path."
    fi
}

main()
{
    MISSING_PACKAGES=()
    Info "Filling Oryn WSL host prerequisites."

    NeedCommand clang clang
    NeedCommand ld.lld lld
    NeedCommand llvm-objcopy llvm
    NeedCommand qemu-system-x86_64 qemu-system-x86
    NeedCommand mkfs.vfat dosfstools
    NeedCommand mcopy mtools
    NeedCommand zip zip
    NeedCommand unzip unzip
    NeedCommand git git

    InstallMissingPackages
    CheckWindowsQemu
    Ok "Prerequisite filling completed."
}

main "$@"
