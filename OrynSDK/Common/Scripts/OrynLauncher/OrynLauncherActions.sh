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

    if AskYesNoShell "Expose and test APIC2/x2APIC as its own profile?" "$(case "$CurrentAPIC2" in no|No|NO|off|Off|OFF|false|False|FALSE|0) echo 0 ;; *) echo 1 ;; esac)"; then
        NewAPIC2="on"
    else
        NewAPIC2="off"
    fi

    if AskYesNoShell "Expose and test HPET?" "$(case "$CurrentHPET" in no|No|NO|off|Off|OFF|false|False|FALSE|0) echo 0 ;; *) echo 1 ;; esac)"; then
        NewHPET="on"
    else
        NewHPET="off"
    fi

    if [ "$NewSMP" -gt 1 ] && [ "$NewAPIC" != "on" ] && [ "$NewAPIC2" != "on" ]; then
        Warn "Multi-core AP startup needs APIC or APIC2/x2APIC. Saving SMP=1 because both are off."
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

