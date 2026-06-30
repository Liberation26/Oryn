#!/usr/bin/env bash
set -u

ScriptPath="${BASH_SOURCE[0]}"
while [ -L "$ScriptPath" ]; do
    ScriptDir="$(cd "$(dirname "$ScriptPath")" && pwd)"
    ScriptPath="$(readlink "$ScriptPath")"
    case "$ScriptPath" in
        /*) ;;
        *) ScriptPath="$ScriptDir/$ScriptPath" ;;
    esac
done

SdkRoot="$(cd "$(dirname "$ScriptPath")" && pwd)"
WorkspaceRoot="$(cd "$SdkRoot/.." && pwd)"
OrynBin="$SdkRoot/Common/Bin/oryn"
BuildScript="$SdkRoot/Common/Scripts/BuildOryn.sh"
DefaultProject="$WorkspaceRoot/OrynProjects/Kernel-5/Project.oryn"

ColorReset="\033[0m"
ColorInfo="\033[36m"
ColorOk="\033[32m"
ColorWarn="\033[33m"
ColorFail="\033[31m"
UseColor=1
[ -n "${NO_COLOR:-}" ] && UseColor=0
[ "${ORYN_NO_COLOR:-0}" = "1" ] && UseColor=0

PrintTag()
{
    Color="$1"
    Tag="$2"
    Message="$3"
    Stream="${4:-1}"

    if [ "$UseColor" -eq 1 ]; then
        if [ "$Stream" = "2" ]; then
            printf "%b%s%b %s\n" "$Color" "$Tag" "$ColorReset" "$Message" >&2
        else
            printf "%b%s%b %s\n" "$Color" "$Tag" "$ColorReset" "$Message"
        fi
    else
        if [ "$Stream" = "2" ]; then
            printf "%s %s\n" "$Tag" "$Message" >&2
        else
            printf "%s %s\n" "$Tag" "$Message"
        fi
    fi
}

Info() { PrintTag "$ColorInfo" "[INFO]" "$1"; }
Ok() { PrintTag "$ColorOk" "[ OK ]" "$1"; }
Warn() { PrintTag "$ColorWarn" "[WARN]" "$1"; }
Fail() { PrintTag "$ColorFail" "[FAIL]" "$1" 2; }

AskYesNoShell()
{
    Question="$1"
    DefaultYes="$2"

    while true; do
        if [ "$DefaultYes" -eq 1 ]; then
            printf '%s [Y/n]: ' "$Question"
        else
            printf '%s [y/N]: ' "$Question"
        fi
        IFS= read -r Answer || Answer=""
        Answer="$(printf '%s' "$Answer" | tr -d '\r' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
        case "$Answer" in
            '') return "$((DefaultYes ? 0 : 1))" ;;
            y|Y|yes|YES|Yes) return 0 ;;
            n|N|no|NO|No) return 1 ;;
            *) Warn "Please answer y or n." ;;
        esac
    done
}

ProjectRunDisplay()
{
    ProjectFile="$1"
    awk '
        BEGIN { section=""; display="" }
        /^[[:space:]]*\[/ {
            section=$0
            gsub(/^[[:space:]]*\[/, "", section)
            gsub(/\][[:space:]]*$/, "", section)
            next
        }
        section == "Run" && /^[[:space:]]*Display[[:space:]]*=/ {
            sub(/^[[:space:]]*Display[[:space:]]*=[[:space:]]*/, "")
            gsub(/\r/, "")
            display=$0
        }
        END { print display }
    ' "$ProjectFile"
}

WriteProjectRunDisplay()
{
    ProjectFile="$1"
    DisplayValue="$2"
    TempFile="$(mktemp)"

    awk -v display="$DisplayValue" '
        BEGIN { section=""; saw_run=0; wrote_display=0 }
        /^[[:space:]]*\[/ {
            if (section == "Run" && wrote_display == 0) {
                print "Display=" display
                wrote_display=1
            }
            section=$0
            gsub(/^[[:space:]]*\[/, "", section)
            gsub(/\][[:space:]]*$/, "", section)
            gsub(/\r/, "", section)
            if (section == "Run") {
                saw_run=1
            }
            print
            next
        }
        section == "Run" && /^[[:space:]]*Display[[:space:]]*=/ {
            if (wrote_display == 0) {
                print "Display=" display
                wrote_display=1
            }
            next
        }
        { print }
        END {
            if (saw_run == 0) {
                print ""
                print "[Run]"
                print "VM=QEMU"
                print "Memory=512M"
                print "Serial=stdio"
                print "Display=" display
            } else if (section == "Run" && wrote_display == 0) {
                print "Display=" display
            }
        }
    ' "$ProjectFile" > "$TempFile" || {
        rm -f "$TempFile"
        return 1
    }

    cat "$TempFile" > "$ProjectFile"
    rm -f "$TempFile"
    return 0
}



ProjectRunValue()
{
    ProjectFile="$1"
    Key="$2"
    DefaultValue="${3:-}"
    awk -v wanted="$Key" -v default_value="$DefaultValue" '
        BEGIN { section=""; value=default_value }
        /^[[:space:]]*\[/ {
            section=$0
            gsub(/^[[:space:]]*\[/, "", section)
            gsub(/\][[:space:]]*$/, "", section)
            gsub(/\r/, "", section)
            next
        }
        section == "Run" {
            line=$0
            gsub(/\r/, "", line)
            split(line, parts, "=")
            key=parts[1]
            gsub(/^[[:space:]]*/, "", key)
            gsub(/[[:space:]]*$/, "", key)
            if (key == wanted) {
                sub(/^[^=]*=[[:space:]]*/, "", line)
                value=line
            }
        }
        END { print value }
    ' "$ProjectFile"
}

WriteProjectRunSetting()
{
    ProjectFile="$1"
    Key="$2"
    Value="$3"
    TempFile="$(mktemp)"

    awk -v key="$Key" -v value="$Value" '
        BEGIN { section=""; saw_run=0; wrote_key=0 }
        /^[[:space:]]*\[/ {
            if (section == "Run" && wrote_key == 0) {
                print key "=" value
                wrote_key=1
            }
            section=$0
            gsub(/^[[:space:]]*\[/, "", section)
            gsub(/\][[:space:]]*$/, "", section)
            gsub(/\r/, "", section)
            if (section == "Run") {
                saw_run=1
            }
            print
            next
        }
        section == "Run" {
            line=$0
            gsub(/\r/, "", line)
            probe=line
            split(probe, parts, "=")
            current=parts[1]
            gsub(/^[[:space:]]*/, "", current)
            gsub(/[[:space:]]*$/, "", current)
            if (current == key) {
                if (wrote_key == 0) {
                    print key "=" value
                    wrote_key=1
                }
                next
            }
        }
        { print }
        END {
            if (saw_run == 0) {
                print ""
                print "[Run]"
                print key "=" value
            } else if (section == "Run" && wrote_key == 0) {
                print key "=" value
            }
        }
    ' "$ProjectFile" > "$TempFile" || {
        rm -f "$TempFile"
        return 1
    }

    cat "$TempFile" > "$ProjectFile"
    rm -f "$TempFile"
    return 0
}

IsValidVmValueShell()
{
    Candidate="$1"
    case "$Candidate" in
        ''|*[!ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_+.,:=/-]*) return 1 ;;
        *) return 0 ;;
    esac
}

AskValueShell()
{
    Question="$1"
    DefaultValue="$2"
    Validator="$3"

    while true; do
        printf '%s [%s]: ' "$Question" "${DefaultValue:-<empty>}" >&2
        IFS= read -r Answer || Answer=""
        Answer="$(printf '%s' "$Answer" | tr -d '\r' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
        [ -n "$Answer" ] || Answer="$DefaultValue"
        [ "$Answer" = "<empty>" ] && Answer=""

        case "$Validator" in
            vm)
                if IsValidVmValueShell "$Answer"; then
                    printf '%s\n' "$Answer"
                    return 0
                fi
                printf '[WARN] Use letters, numbers, underscore, dash, plus, comma, dot, colon, slash or equals only.\n' >&2
                ;;
            number)
                case "$Answer" in
                    ''|*[!0123456789]*) printf '[WARN] Use a whole number.\n' >&2 ;;
                    *) printf '%s\n' "$Answer"; return 0 ;;
                esac
                ;;
            *)
                printf '%s\n' "$Answer"
                return 0
                ;;
        esac
    done
}


NormalizeQemuCpuForWslShell()
{
    CpuValue="$1"
    case "$CpuValue" in
        host|Host|HOST|native|Native|NATIVE)
            PrintTag "$ColorWarn" "[WARN]" "CPU=$CpuValue requires KVM/HVF/WHX acceleration and is not valid for the current Windows QEMU from WSL runner." 2
            PrintTag "$ColorWarn" "[WARN]" "Saving CPU=max instead. Use qemu64 for the most conservative profile." 2
            printf 'max\n'
            ;;
        '')
            printf 'qemu64\n'
            ;;
        *)
            printf '%s\n' "$CpuValue"
            ;;
    esac
}

RunVMSettingsQuestionnaire()
{
    ProjectFile="$1"
    shift || true

    if [ ! -f "$ProjectFile" ]; then
        Fail "Project was not found: $ProjectFile"
        exit 1
    fi

    CurrentVM="$(ProjectRunValue "$ProjectFile" VM QEMU)"
    CurrentFormatVM="$(ProjectRunValue "$ProjectFile" FormatVM yes)"
    CurrentDisplay="$(ProjectRunValue "$ProjectFile" Display none)"
    CurrentMemory="$(ProjectRunValue "$ProjectFile" Memory 512M)"
    CurrentCPU="$(ProjectRunValue "$ProjectFile" CPU qemu64)"
    CurrentSMP="$(ProjectRunValue "$ProjectFile" SMP 4)"
    CurrentPIC="$(ProjectRunValue "$ProjectFile" PIC on)"
    CurrentAPIC="$(ProjectRunValue "$ProjectFile" APIC on)"
    CurrentAPIC2="$(ProjectRunValue "$ProjectFile" APIC2 on)"
    CurrentHPET="$(ProjectRunValue "$ProjectFile" HPET on)"
    CurrentDiskFormat="$(ProjectRunValue "$ProjectFile" DiskFormat raw)"
    CurrentStorage="$(ProjectRunValue "$ProjectFile" StorageInterface ide)"

    Info "VMSettings questionnaire."
    Info "This saves the QEMU VM profile into the project [Run] section."

    NewVM="$(AskValueShell 'VM provider' "$CurrentVM" vm)"
    case "$NewVM" in
        QEMU|qemu) NewVM="QEMU" ;;
        *) Warn "Only QEMU is currently supported by Oryn WSL run; saving VM=QEMU."; NewVM="QEMU" ;;
    esac

    if AskYesNoShell "Format/recreate the FAT32 VM disk image when running?" "$(case "$CurrentFormatVM" in no|No|NO|off|Off|OFF|false|False|FALSE|0) echo 0 ;; *) echo 1 ;; esac)"; then
        NewFormatVM="yes"
    else
        NewFormatVM="no"
    fi

    NewDiskFormat="$(AskValueShell 'VM disk image format' "$CurrentDiskFormat" vm)"
    case "$NewDiskFormat" in
        raw|RAW|Raw) NewDiskFormat="raw" ;;
        *) Warn "The built-in image writer currently supports raw only; saving DiskFormat=raw."; NewDiskFormat="raw" ;;
    esac

    NewStorage="$(AskValueShell 'VM storage interface' "$CurrentStorage" vm)"
    case "$NewStorage" in
        ide|IDE|Ide) NewStorage="ide" ;;
        *) Warn "The current UEFI FAT32 runner supports ide only; saving StorageInterface=ide."; NewStorage="ide" ;;
    esac

    if AskYesNoShell "Run this kernel VM headless, without a QEMU window?" "$(case "$CurrentDisplay" in ''|none|None|NONE|headless|Headless|HEADLESS|yes|Yes|YES|true|True|TRUE) echo 1 ;; *) echo 0 ;; esac)"; then
        NewDisplay="none"
    else
        NewDisplay="sdl"
    fi

    NewMemory="$(AskValueShell 'VM memory, for example 512M, 1G, 2G' "$CurrentMemory" vm)"
    NewCPU="$(AskValueShell 'QEMU CPU model base, for example qemu64 or max' "$CurrentCPU" vm)"
    NewCPU="$(NormalizeQemuCpuForWslShell "$NewCPU")"
    NewSMP="$(AskValueShell 'VM CPU/core count' "$CurrentSMP" number)"
    [ "$NewSMP" -lt 1 ] && NewSMP=1
    if [ "$NewSMP" -gt 64 ]; then
        Warn "Capping VM CPU/core count at 64."
        NewSMP=64
    fi

    if AskYesNoShell "Expose and test the legacy PIC path?" "$(case "$CurrentPIC" in no|No|NO|off|Off|OFF|false|False|FALSE|0) echo 0 ;; *) echo 1 ;; esac)"; then
        NewPIC="on"
    else
        NewPIC="off"
    fi

    if AskYesNoShell "Expose and test Local APIC?" "$(case "$CurrentAPIC" in no|No|NO|off|Off|OFF|false|False|FALSE|0) echo 0 ;; *) echo 1 ;; esac)"; then
        NewAPIC="on"
    else
        NewAPIC="off"
    fi

    if [ "$NewAPIC" = "on" ]; then
        if AskYesNoShell "Expose and prefer APIC2/x2APIC?" "$(case "$CurrentAPIC2" in no|No|NO|off|Off|OFF|false|False|FALSE|0) echo 0 ;; *) echo 1 ;; esac)"; then
            NewAPIC2="on"
        else
            NewAPIC2="off"
        fi
    else
        NewAPIC2="off"
        Warn "APIC2/x2APIC requires APIC, so APIC2 was saved as off."
    fi

    if AskYesNoShell "Expose and test HPET?" "$(case "$CurrentHPET" in no|No|NO|off|Off|OFF|false|False|FALSE|0) echo 0 ;; *) echo 1 ;; esac)"; then
        NewHPET="on"
    else
        NewHPET="off"
    fi

    if [ "$NewSMP" -gt 1 ] && [ "$NewAPIC" != "on" ]; then
        Warn "Multi-core AP startup needs Local APIC. Saving SMP=1 because APIC is off."
        NewSMP=1
    fi

    for Pair in \
        "VM=$NewVM" \
        "FormatVM=$NewFormatVM" \
        "DiskFormat=$NewDiskFormat" \
        "StorageInterface=$NewStorage" \
        "Display=$NewDisplay" \
        "Memory=$NewMemory" \
        "CPU=$NewCPU" \
        "SMP=$NewSMP" \
        "PIC=$NewPIC" \
        "APIC=$NewAPIC" \
        "APIC2=$NewAPIC2" \
        "HPET=$NewHPET"
    do
        Key="${Pair%%=*}"
        Value="${Pair#*=}"
        if ! WriteProjectRunSetting "$ProjectFile" "$Key" "$Value"; then
            Fail "Could not save VMSettings key $Key to: $ProjectFile"
            exit 1
        fi
    done

    Ok "Saved VMSettings to project [Run]."
    Info "VMSettings summary: VM=$NewVM FormatVM=$NewFormatVM DiskFormat=$NewDiskFormat StorageInterface=$NewStorage"
    Info "VMSettings summary: Display=$NewDisplay Memory=$NewMemory CPU=$NewCPU SMP=$NewSMP"
    Info "VMSettings summary: PIC=$NewPIC APIC=$NewAPIC APIC2=$NewAPIC2 HPET=$NewHPET"

    case "${ORYN_VMSETTINGS_CONTINUE:-}" in
        BootConfig|bootconfig)
            Info "Continuing to BootConfig questionnaire."
            RunBootConfigQuestionnaire "$ProjectFile" "$@"
            ;;
        *)
            Info "Use ./Oryn.sh run to build/image/run with these settings."
            ;;
    esac
}


ProjectOSName()
{
    ProjectFile="$1"
    awk '
        BEGIN { name="" }
        /^[[:space:]]*Name[[:space:]]*=/ {
            sub(/^[[:space:]]*Name[[:space:]]*=[[:space:]]*/, "")
            gsub(/\r/, "")
            name=$0
        }
        END { print name }
    ' "$ProjectFile"
}

IsValidOSNameShell()
{
    Candidate="$1"
    case "$Candidate" in
        ''|*[!ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-]*) return 1 ;;
        *) return 0 ;;
    esac
}

WriteProjectOSName()
{
    ProjectFile="$1"
    OSName="$2"
    TempFile="$(mktemp)"

    awk -v os_name="$OSName" '
        BEGIN { wrote_name=0 }
        /^[[:space:]]*Name[[:space:]]*=/ {
            if (wrote_name == 0) {
                print "Name=" os_name
                wrote_name=1
            }
            next
        }
        { print }
        END {
            if (wrote_name == 0) {
                print "Name=" os_name
            }
        }
    ' "$ProjectFile" > "$TempFile" || {
        rm -f "$TempFile"
        return 1
    }

    cat "$TempFile" > "$ProjectFile"
    rm -f "$TempFile"
    return 0
}


ProjectKernelCommandLine()
{
    ProjectFile="$1"
    awk '
        BEGIN { section=""; command_line="" }
        /^[[:space:]]*\[/ {
            section=$0
            gsub(/^[[:space:]]*\[/, "", section)
            gsub(/\][[:space:]]*$/, "", section)
            gsub(/\r/, "", section)
            next
        }
        section == "BootConfiguration" && /^[[:space:]]*CommandLine[[:space:]]*=/ {
            sub(/^[[:space:]]*CommandLine[[:space:]]*=[[:space:]]*/, "")
            gsub(/\r/, "")
            command_line=$0
        }
        END { print command_line }
    ' "$ProjectFile"
}

WriteProjectKernelCommandLine()
{
    ProjectFile="$1"
    CommandLineValue="$2"
    TempFile="$(mktemp)"

    awk -v command_line="$CommandLineValue" '
        BEGIN { section=""; saw_boot=0; wrote_command_line=0 }
        /^[[:space:]]*\[/ {
            if (section == "BootConfiguration" && wrote_command_line == 0) {
                print "CommandLine=" command_line
                wrote_command_line=1
            }
            section=$0
            gsub(/^[[:space:]]*\[/, "", section)
            gsub(/\][[:space:]]*$/, "", section)
            gsub(/\r/, "", section)
            if (section == "BootConfiguration") {
                saw_boot=1
            }
            print
            next
        }
        section == "BootConfiguration" && /^[[:space:]]*CommandLine[[:space:]]*=/ {
            if (wrote_command_line == 0) {
                print "CommandLine=" command_line
                wrote_command_line=1
            }
            next
        }
        { print }
        END {
            if (saw_boot == 0) {
                print ""
                print "[BootConfiguration]"
                print "CommandLine=" command_line
            } else if (section == "BootConfiguration" && wrote_command_line == 0) {
                print "CommandLine=" command_line
            }
        }
    ' "$ProjectFile" > "$TempFile" || {
        rm -f "$TempFile"
        return 1
    }

    cat "$TempFile" > "$ProjectFile"
    rm -f "$TempFile"
    return 0
}

RunBootConfigQuestionnaire()
{
    ProjectFile="$1"
    shift || true

    if [ ! -f "$ProjectFile" ]; then
        Fail "Project was not found: $ProjectFile"
        exit 1
    fi

    CurrentCommandLine="$(ProjectKernelCommandLine "$ProjectFile")"

    Info "Kernel boot configuration questionnaire."
    Info "Current kernel command line: ${CurrentCommandLine:-<empty>}"
    printf 'Kernel command line [%s]: ' "${CurrentCommandLine:-<empty>}"
    IFS= read -r NewCommandLine || NewCommandLine=""
    NewCommandLine="$(printf '%s' "$NewCommandLine" | tr -d '\r' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
    [ "$NewCommandLine" = "<empty>" ] && NewCommandLine=""
    [ -n "$NewCommandLine" ] || NewCommandLine="$CurrentCommandLine"

    if [ "${#NewCommandLine}" -ge 256 ]; then
        Fail "Kernel command line must be shorter than 256 bytes for the current BootInfo ABI."
        exit 1
    fi

    if ! WriteProjectKernelCommandLine "$ProjectFile" "$NewCommandLine"; then
        Fail "Could not save the kernel command line to: $ProjectFile"
        exit 1
    fi

    Ok "Saved kernel command line: ${NewCommandLine:-<empty>}"
    Info "Continuing to BootInfo questionnaire."
    exec "$OrynBin" bootinfo "$ProjectFile" "$@"
}

RunOSNameQuestionnaire()
{
    ProjectFile="$1"
    shift || true

    if [ ! -f "$ProjectFile" ]; then
        Fail "Project was not found: $ProjectFile"
        exit 1
    fi

    CurrentName="$(ProjectOSName "$ProjectFile")"
    [ -n "$CurrentName" ] || CurrentName="Kernel-5"

    Info "Kernel/OS name questionnaire."
    Info "Current project OS name: $CurrentName"
    while true; do
        printf 'What is this kernel/OS called? [%s]: ' "$CurrentName"
        IFS= read -r NewName || NewName=""
        NewName="$(printf '%s' "$NewName" | tr -d '\r' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
        [ -n "$NewName" ] || NewName="$CurrentName"

        if IsValidOSNameShell "$NewName"; then
            break
        fi

        Warn "Use only letters, numbers, dash, and underscore for the kernel/OS name."
    done

    if ! WriteProjectOSName "$ProjectFile" "$NewName"; then
        Fail "Could not save the kernel/OS name to: $ProjectFile"
        exit 1
    fi

    Ok "Saved kernel/OS name: $NewName"
    Info "Continuing to VMSettings questionnaire."
    ORYN_VMSETTINGS_CONTINUE=BootConfig RunVMSettingsQuestionnaire "$ProjectFile" "$@"
}

RunHeadlessQuestionnaire()
{
    ProjectFile="$1"
    shift || true

    if [ ! -f "$ProjectFile" ]; then
        Fail "Project was not found: $ProjectFile"
        exit 1
    fi

    CurrentDisplay="$(ProjectRunDisplay "$ProjectFile")"
    case "$CurrentDisplay" in
        ''|none|None|NONE|headless|Headless|HEADLESS|yes|Yes|YES|true|True|TRUE)
            DefaultHeadless=1
            ;;
        *)
            DefaultHeadless=0
            ;;
    esac

    Info "VM display questionnaire."
    Info "Current project Display setting: ${CurrentDisplay:-none}"
    if AskYesNoShell "Run this kernel VM headless, without a QEMU window?" "$DefaultHeadless"; then
        NewDisplay="none"
        HeadlessText="yes"
    else
        NewDisplay="sdl"
        HeadlessText="no"
    fi

    if ! WriteProjectRunDisplay "$ProjectFile" "$NewDisplay"; then
        Fail "Could not save the VM display setting to: $ProjectFile"
        exit 1
    fi

    Ok "Saved VM headless mode: $HeadlessText"
    Ok "Saved project [Run] Display=$NewDisplay"
    Info "Continuing to BootConfig questionnaire."
    RunBootConfigQuestionnaire "$ProjectFile" "$@"
}


GetBootInfoVariantMask()
{
    HeaderPath="$1/Include/OrynBootInfoSelection.h"
    [ -f "$HeaderPath" ] || return 1

    MaskLine="$(grep '^#define ORYN_BOOTINFO_SELECTION_MASK ' "$HeaderPath" 2>/dev/null | head -n 1 | awk '{print $3}')"
    if [ -n "$MaskLine" ]; then
        printf '%s\n' "$MaskLine"
        return 0
    fi

    Value=0
    grep -q '^#define ORYN_BOOTINFO_WANT_KERNEL_RANGE 1' "$HeaderPath" && Value=$((Value | 8))
    grep -q '^#define ORYN_BOOTINFO_WANT_MEMORY_MAP 1' "$HeaderPath" && Value=$((Value | 1))
    grep -q '^#define ORYN_BOOTINFO_WANT_FRAMEBUFFER 1' "$HeaderPath" && Value=$((Value | 2))
    grep -q '^#define ORYN_BOOTINFO_WANT_RSDP 1' "$HeaderPath" && Value=$((Value | 4))
    grep -q '^#define ORYN_BOOTINFO_WANT_FIRMWARE_DATA 1' "$HeaderPath" && Value=$((Value | 32))
    grep -q '^#define ORYN_BOOTINFO_WANT_PLATFORM_TABLES 1' "$HeaderPath" && Value=$((Value | 64))
    grep -q '^#define ORYN_BOOTINFO_WANT_NVRAM 1' "$HeaderPath" && Value=$((Value | 128))
    grep -q '^#define ORYN_BOOTINFO_WANT_RUNTIME_SERVICES 1' "$HeaderPath" && Value=$((Value | 256))
    printf '0x%X\n' "$Value"
    return 0
}

FindVariantByMask()
{
    VariantRoot="$1"
    WantedMask="$2"

    for Candidate in "$VariantRoot"/[0-9]*; do
        [ -d "$Candidate" ] || continue
        CandidateMask="$(GetBootInfoVariantMask "$Candidate" 2>/dev/null || true)"
        [ "$CandidateMask" = "$WantedMask" ] || continue
        basename "$Candidate"
        return 0
    done

    return 1
}

NextKernelVariantNumber()
{
    VariantRoot="$1"
    Highest=0

    for Candidate in "$VariantRoot"/[0-9]*; do
        [ -d "$Candidate" ] || continue
        Number="$(basename "$Candidate")"
        case "$Number" in
            *[!0-9]*|'') continue ;;
        esac
        [ "$Number" -gt "$Highest" ] && Highest="$Number"
    done

    printf '%s\n' $((Highest + 1))
}

RewriteMigratedVariantText()
{
    VariantDir="$1"
    Number="$2"
    HeaderPath="$VariantDir/Include/OrynBootInfoSelection.h"
    NotesPath="$VariantDir/BootInfoSelection.txt"

    [ -f "$HeaderPath" ] && sed -i \
        -e "s/#define ORYN_BOOTINFO_SELECTION_NUMBER .*/#define ORYN_BOOTINFO_SELECTION_NUMBER $Number/" \
        -e "s#Kernel-[0-9][0-9]*/Kernel/[0-9][0-9]*#Kernel-5/Kernel/$Number#g" \
        "$HeaderPath"

    [ -f "$NotesPath" ] && sed -i \
        -e "s/Oryn Kernel-[0-9][0-9]* BootInfo selection [0-9][0-9]*/Oryn Kernel-5 BootInfo selection $Number/g" \
        -e "s#Kernel-[0-9][0-9]*/Kernel/[0-9][0-9]*#Kernel-5/Kernel/$Number#g" \
        "$NotesPath"
}

MigrateLegacyKernelBootInfoVariants()
{
    LegacyName="$1"
    LegacyProject="$WorkspaceRoot/OrynProjects/$LegacyName"
    NewProject="$WorkspaceRoot/OrynProjects/Kernel-5"
    LegacyRoot="$LegacyProject/Kernel"
    NewRoot="$NewProject/Kernel"

    [ -d "$LegacyProject" ] || return 0
    [ -d "$LegacyRoot" ] || return 0
    [ -d "$NewRoot" ] || return 0

    LegacySelected=""
    [ -f "$LegacyRoot/Selected.txt" ] && LegacySelected="$(tr -d '\r\n' < "$LegacyRoot/Selected.txt")"
    NewSelected=""

    for LegacyVariant in "$LegacyRoot"/[0-9]*; do
        [ -d "$LegacyVariant" ] || continue
        LegacyNumber="$(basename "$LegacyVariant")"
        LegacyMask="$(GetBootInfoVariantMask "$LegacyVariant" 2>/dev/null || true)"
        [ -n "$LegacyMask" ] || continue

        ExistingNumber="$(FindVariantByMask "$NewRoot" "$LegacyMask" 2>/dev/null || true)"
        if [ -n "$ExistingNumber" ]; then
            [ "$LegacyNumber" = "$LegacySelected" ] && NewSelected="$ExistingNumber"
            continue
        fi

        NewNumber="$(NextKernelVariantNumber "$NewRoot")"
        cp -a "$LegacyVariant" "$NewRoot/$NewNumber"
        RewriteMigratedVariantText "$NewRoot/$NewNumber" "$NewNumber"
        Ok "Migrated $LegacyName BootInfo variant Kernel/$LegacyNumber to Kernel-5/Kernel/$NewNumber."
        [ "$LegacyNumber" = "$LegacySelected" ] && NewSelected="$NewNumber"
    done

    if [ -n "$NewSelected" ]; then
        printf '%s\n' "$NewSelected" > "$NewRoot/Selected.txt"
        Ok "Preserved selected BootInfo variant as Kernel-5/Kernel/$NewSelected."
    fi
}

RemoveLegacyProjectIfGenerated()
{
    LegacyProject="$1"
    LegacyName="$2"

    [ -f "$DefaultProject" ] || return 0
    [ -d "$LegacyProject" ] || return 0
    [ -f "$LegacyProject/Project.oryn" ] || return 0

    if grep -qx "Name=$LegacyName" "$LegacyProject/Project.oryn" 2>/dev/null; then
        rm -rf "$LegacyProject"
        Ok "Removed old generated $LegacyName project folder. Kernel-5 is now the default project."
    fi
}


RemoveObsoleteSdkSourcePaths()
{
    for ObsoletePath in         "$SdkRoot/Source"         "$SdkRoot/Bin"         "$SdkRoot/Docs"         "$SdkRoot/Scripts"         "$SdkRoot/Common/Boot"; do
        if [ -e "$ObsoletePath" ]; then
            rm -rf "$ObsoletePath"
            Ok "Removed obsolete SDK path: ${ObsoletePath#$SdkRoot/}"
        fi
    done
}


RemoveMovedKernelFile()
{
    MovedPath="$1"

    if [ -e "$MovedPath" ]; then
        rm -rf "$MovedPath"
        Ok "Removed project-owned copy now supplied by SDK: ${MovedPath#$WorkspaceRoot/}"
    fi
}


RemoveMovedSharedKernelProjectPaths()
{
    KernelProject="$WorkspaceRoot/OrynProjects/Kernel-5"
    [ -d "$KernelProject" ] || return 0

    RemoveMovedKernelFile "$KernelProject/Include/KernelBootInfo.h"
    RemoveMovedKernelFile "$KernelProject/Include/KernelConsole.h"
    RemoveMovedKernelFile "$KernelProject/Include/KernelIo.h"
    RemoveMovedKernelFile "$KernelProject/Include/KernelMemoryMap.h"
    RemoveMovedKernelFile "$KernelProject/Include/KernelPhysicalMemory.h"
    RemoveMovedKernelFile "$KernelProject/Include/KernelTtf.h"
    RemoveMovedKernelFile "$KernelProject/Include/KernelVirtualMemory.h"
    RemoveMovedKernelFile "$KernelProject/Include/Serial.h"

    RemoveMovedKernelFile "$KernelProject/Source/BootInfo"
    RemoveMovedKernelFile "$KernelProject/Source/Console"
    RemoveMovedKernelFile "$KernelProject/Source/Fonts"
    RemoveMovedKernelFile "$KernelProject/Source/KernelIo.c"
    RemoveMovedKernelFile "$KernelProject/Source/Serial.c"
    RemoveMovedKernelFile "$KernelProject/Source/Memory/KernelMemoryMap.c"
    RemoveMovedKernelFile "$KernelProject/Source/Memory/KernelMemoryMapPrint.c"
    RemoveMovedKernelFile "$KernelProject/Source/Memory/KernelPhysicalMemory.c"
    RemoveMovedKernelFile "$KernelProject/Source/Memory/KernelPhysicalMemoryPrint.c"
    RemoveMovedKernelFile "$KernelProject/Source/Memory/KernelVirtualMemory.c"
    RemoveMovedKernelFile "$KernelProject/Source/Memory/KernelVirtualMemoryPrint.c"
    find "$KernelProject/Source" -type d -empty -delete 2>/dev/null || true
    find "$KernelProject/Include" -type d -empty -delete 2>/dev/null || true
}


MigrateLegacyKernelBootInfoVariants "Kernel-3"
MigrateLegacyKernelBootInfoVariants "Kernel-4"
RemoveObsoleteSdkSourcePaths
RemoveMovedSharedKernelProjectPaths
RemoveLegacyProjectIfGenerated "$WorkspaceRoot/OrynProjects/TestOS" "TestOS"
RemoveLegacyProjectIfGenerated "$WorkspaceRoot/OrynProjects/Kernel-1" "Kernel-1"
RemoveLegacyProjectIfGenerated "$WorkspaceRoot/OrynProjects/Kernel-2" "Kernel-2"
RemoveLegacyProjectIfGenerated "$WorkspaceRoot/OrynProjects/Kernel-3" "Kernel-3"
RemoveLegacyProjectIfGenerated "$WorkspaceRoot/OrynProjects/Kernel-4" "Kernel-4"

if [ "$#" -gt 0 ]; then
    case "$1" in
        GitPush|gitpush|push)
            shift || true
            exec bash "$SdkRoot/Common/Scripts/GitPushOryn.sh" "$@"
            ;;
        GitLogin|gitlogin|gcm|credentials)
            shift || true
            exec bash "$SdkRoot/Common/Scripts/GitPushOryn.sh" --setup-gcm-only "$@"
            ;;
    esac
fi

NeedBuild=0
if [ ! -x "$OrynBin" ]; then
    NeedBuild=1
elif find "$SdkRoot/Common/OrynBuild" -type f \( -name '*.c' -o -name '*.h' \) -newer "$OrynBin" 2>/dev/null | grep -q .; then
    NeedBuild=1
elif find "$SdkRoot/Targets/UEFI/X64/OrynBuild" -type f \( -name '*.c' -o -name '*.h' \) -newer "$OrynBin" 2>/dev/null | grep -q .; then
    NeedBuild=1
elif [ -f "$SdkRoot/VERSION" ]; then
    ExpectedVersion="$(tr -d '\r\n' < "$SdkRoot/VERSION")"
    CurrentVersion="$("$OrynBin" version 2>/dev/null | awk '{print $4}' | head -n 1)"
    if [ "$ExpectedVersion" != "$CurrentVersion" ]; then
        NeedBuild=1
    fi
fi

if [ "$NeedBuild" -eq 1 ]; then
    if [ ! -f "$BuildScript" ]; then
        Fail "Build script was not found: $BuildScript"
        exit 1
    fi

    Info "Bootstrapping native Oryn build tool."
    bash "$BuildScript" || exit $?
fi

if [ ! -x "$OrynBin" ]; then
    Fail "Native Oryn build tool was not produced: $OrynBin"
    exit 1
fi

RunDefaultProject()
{
    if [ ! -f "$DefaultProject" ]; then
        Fail "Default Kernel-5 project was not found: $DefaultProject"
        Warn "Expected workspace layout:"
        Warn "  $WorkspaceRoot/OrynSDK"
        Warn "  $WorkspaceRoot/OrynProjects/Kernel-5/Project.oryn"
        Warn "Use: ./Oryn.sh run /path/to/Project.oryn"
        exit 1
    fi

    exec "$OrynBin" run "$DefaultProject"
}

if [ "$#" -eq 0 ]; then
    RunDefaultProject
fi

Command="$1"
shift || true

case "$Command" in
    rebuild|bootstrap)
        exec bash "$BuildScript"
        ;;

    doctor|version)
        exec "$OrynBin" "$Command" "$@"
        ;;

    build|image|run|clean)
        if [ "$#" -eq 0 ]; then
            if [ ! -f "$DefaultProject" ]; then
                Fail "Default Kernel-5 project was not found: $DefaultProject"
                Warn "Expected workspace layout:"
                Warn "  $WorkspaceRoot/OrynSDK"
                Warn "  $WorkspaceRoot/OrynProjects/Kernel-5/Project.oryn"
                Warn "Use: ./Oryn.sh $Command /path/to/Project.oryn"
                exit 1
            fi
            exec "$OrynBin" "$Command" "$DefaultProject"
        fi
        exec "$OrynBin" "$Command" "$@"
        ;;

    OSName|osname|name)
        if [ "$#" -eq 0 ]; then
            RunOSNameQuestionnaire "$DefaultProject"
        fi

        case "$1" in
            *.oryn)
                ProjectFile="$1"
                shift || true
                RunOSNameQuestionnaire "$ProjectFile" "$@"
                ;;
            *)
                RunOSNameQuestionnaire "$DefaultProject" "$@"
                ;;
        esac
        ;;

    VMSettings|vmsettings|VM|vm)
        if [ "$#" -eq 0 ]; then
            RunVMSettingsQuestionnaire "$DefaultProject"
            exit 0
        fi

        case "$1" in
            *.oryn)
                ProjectFile="$1"
                shift || true
                RunVMSettingsQuestionnaire "$ProjectFile" "$@"
                exit 0
                ;;
            *)
                RunVMSettingsQuestionnaire "$DefaultProject" "$@"
                exit 0
                ;;
        esac
        ;;

    Headless|headless)
        if [ "$#" -eq 0 ]; then
            RunHeadlessQuestionnaire "$DefaultProject"
        fi

        case "$1" in
            *.oryn)
                ProjectFile="$1"
                shift || true
                RunHeadlessQuestionnaire "$ProjectFile" "$@"
                ;;
            *)
                RunHeadlessQuestionnaire "$DefaultProject" "$@"
                ;;
        esac
        ;;


    BootConfig|bootconfig|CommandLine|commandline)
        if [ "$#" -eq 0 ]; then
            RunBootConfigQuestionnaire "$DefaultProject"
        fi

        case "$1" in
            *.oryn)
                ProjectFile="$1"
                shift || true
                RunBootConfigQuestionnaire "$ProjectFile" "$@"
                ;;
            *)
                RunBootConfigQuestionnaire "$DefaultProject" "$@"
                ;;
        esac
        ;;

    bootinfo)
        if [ ! -f "$DefaultProject" ]; then
            Fail "Default Kernel-5 project was not found: $DefaultProject"
            Warn "Expected workspace layout:"
            Warn "  $WorkspaceRoot/OrynSDK"
            Warn "  $WorkspaceRoot/OrynProjects/Kernel-5/Project.oryn"
            Warn "Use: ./Oryn.sh bootinfo /path/to/Project.oryn"
            exit 1
        fi

        if [ "$#" -eq 0 ]; then
            exec "$OrynBin" bootinfo "$DefaultProject"
        fi

        case "$1" in
            *.oryn)
                exec "$OrynBin" bootinfo "$@"
                ;;
            *)
                exec "$OrynBin" bootinfo "$DefaultProject" "$@"
                ;;
        esac
        ;;

    help|--help|-h)
        printf 'Oryn WSL SDK launcher\n\n'
        printf 'Workspace root:\n'
        printf '  %s\n\n' "$WorkspaceRoot"
        printf 'SDK root:\n'
        printf '  %s\n\n' "$SdkRoot"
        printf 'Default project:\n'
        printf '  %s\n\n' "$DefaultProject"
        printf 'Usage:\n'
        printf '  ./Oryn.sh                 Build, image, and run Kernel-5\n'
        printf '  ./Oryn.sh doctor          Check tools\n'
        printf '  ./Oryn.sh version         Show SDK version\n'
        printf '  ./Oryn.sh build [project] Build project, defaulting to Kernel-5\n'
        printf '  ./Oryn.sh image [project] Build image, defaulting to Kernel-5\n'
        printf '  ./Oryn.sh run [project]   Run project, defaulting to Kernel-5\n'
        printf '  ./Oryn.sh OSName [project]   Ask kernel/OS name first, then Headless, then BootInfo\n'
        printf '  ./Oryn.sh VMSettings [project] Ask and save VM format, CPU, memory, PIC/APIC/APIC2/HPET settings\n'
        printf '  ./Oryn.sh Headless [project] Ask whether the VM is headless, save Display, then ask BootInfo\n'
        printf '  ./Oryn.sh clean [project] Clean project, defaulting to Kernel-5\n'
        printf '  ./Oryn.sh bootinfo [project] Ask which UEFI BootInfo items to pass, then build and run\n'
        printf '  ./Oryn.sh bootinfo list      List Kernel-5 BootInfo variants\n'
        printf '  ./Oryn.sh bootinfo show [n]  Show selected or numbered variant\n'
        printf '  ./Oryn.sh bootinfo select n  Select an existing variant\n'
        printf '  ./Oryn.sh bootinfo compare a b Compare two variants\n'
        printf '  ./Oryn.sh bootinfo run n     Select, build, image, and run a variant\n'
        printf '  ./Oryn.sh bootinfo test-all  Build, image, and run every variant\n'
        printf '  ./Oryn.sh gitpush         Initialise git, ignore output, commit, and push source\n'
        printf '  ./Oryn.sh gitlogin        Configure Git Credential Manager for WSL GitHub login\n'
        printf '  ./Oryn.sh rebuild         Rebuild Common/Bin/oryn\n'
        exit 0
        ;;

    *.oryn)
        exec "$OrynBin" run "$Command" "$@"
        ;;

    *)
        exec "$OrynBin" "$Command" "$@"
        ;;
esac
