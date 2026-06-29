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

Info() { printf '\033[36m[INFO]\033[0m %s\n' "$1"; }
Ok() { printf '\033[32m[ OK ]\033[0m %s\n' "$1"; }
Warn() { printf '\033[33m[WARN]\033[0m %s\n' "$1"; }
Fail() { printf '\033[31m[FAIL]\033[0m %s\n' "$1" >&2; }

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
    Info "Continuing to BootInfo questionnaire."
    exec "$OrynBin" bootinfo "$ProjectFile" "$@"
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


MigrateLegacyKernelBootInfoVariants "Kernel-3"
MigrateLegacyKernelBootInfoVariants "Kernel-4"
RemoveObsoleteSdkSourcePaths
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
