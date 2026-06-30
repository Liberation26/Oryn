#!/usr/bin/env bash
set -u

ColorReset="\033[0m"
ColorOk="\033[32m"
ColorWarn="\033[33m"
ColorFail="\033[31m"
ColorInfo="\033[36m"

Info() { printf "%b[INFO]%b %s\n" "$ColorInfo" "$ColorReset" "$1"; }
Ok() { printf "%b[ OK ]%b %s\n" "$ColorOk" "$ColorReset" "$1"; }
Warn() { printf "%b[WARN]%b %s\n" "$ColorWarn" "$ColorReset" "$1"; }
Fail() { printf "%b[FAIL]%b %s\n" "$ColorFail" "$ColorReset" "$1" >&2; }

RequireCommand()
{
    if ! command -v "$1" >/dev/null 2>&1; then
        Fail "Required command not found: $1"
        exit 1
    fi
}

CanonicalPath()
{
    local path="$1"
    if [ -d "$path" ]; then
        (cd "$path" && pwd)
        return
    fi

    local dir
    local file
    dir="$(dirname "$path")"
    file="$(basename "$path")"
    (cd "$dir" 2>/dev/null && printf "%s/%s\n" "$(pwd)" "$file")
}

FindNewestZip()
{
    local candidates_file="$1"
    local search_root
    : > "$candidates_file"

    for search_root in \
        "$PWD" \
        "$HOME" \
        "$HOME/Downloads" \
        "$WorkspaceRoot" \
        "$SdkRoot"; do
        [ -d "$search_root" ] || continue
        find "$search_root" -maxdepth 1 -type f -name 'OrynWsl-*.zip' -printf '%T@ %p\n' 2>/dev/null >> "$candidates_file"
    done

    if [ -d /mnt/c/Users ]; then
        find /mnt/c/Users -maxdepth 3 -type f -path '*/Downloads/OrynWsl-*.zip' -printf '%T@ %p\n' 2>/dev/null >> "$candidates_file"
    fi

    sort -nr "$candidates_file" | awk 'NR == 1 { sub(/^[^ ]+ /, ""); print; }'
}

CopyTree()
{
    local source_dir="$1"
    local target_dir="$2"

    [ -d "$source_dir" ] || return 0
    mkdir -p "$target_dir"
    cp -a "$source_dir/." "$target_dir/"
}


RemoveLegacyProjectIfGenerated()
{
    local legacy_project="$1"
    local legacy_name="$2"
    local kernel_project="$ProjectsRoot/Kernel-5"

    [ -d "$legacy_project" ] || return 0
    [ -f "$legacy_project/Project.oryn" ] || return 0
    [ -d "$kernel_project" ] || return 0

    if grep -qx "Name=$legacy_name" "$legacy_project/Project.oryn" 2>/dev/null; then
        rm -rf "$legacy_project"
        Ok "Removed old generated $legacy_name project folder. Kernel-5 is now the default project."
    fi
}

RemoveLegacyGeneratedProjects()
{
    RemoveLegacyProjectIfGenerated "$ProjectsRoot/TestOS" "TestOS"
    RemoveLegacyProjectIfGenerated "$ProjectsRoot/Kernel-1" "Kernel-1"
    RemoveLegacyProjectIfGenerated "$ProjectsRoot/Kernel-2" "Kernel-2"
    RemoveLegacyProjectIfGenerated "$ProjectsRoot/Kernel-3" "Kernel-3"
    RemoveLegacyProjectIfGenerated "$ProjectsRoot/Kernel-4" "Kernel-4"
}

CopyWorkspaceFile()
{
    local source_root="$1"
    local name="$2"

    [ -f "$source_root/$name" ] || return 0
    mkdir -p "$WorkspaceRoot"
    cp -a "$source_root/$name" "$WorkspaceRoot/$name"
}

CopyWorkspaceFileIfMissing()
{
    local source_root="$1"
    local name="$2"

    [ -f "$source_root/$name" ] || return 0
    [ ! -e "$WorkspaceRoot/$name" ] || return 0
    mkdir -p "$WorkspaceRoot"
    cp -a "$source_root/$name" "$WorkspaceRoot/$name"
}

CopyWorkspaceShellAndGitFiles()
{
    local source_root="$1"

    CopyWorkspaceFile "$source_root" Oryn.sh
    CopyWorkspaceFile "$source_root" oryn
    CopyWorkspaceFile "$source_root" update
    CopyWorkspaceFile "$source_root" update.sh
    CopyWorkspaceFile "$source_root" GitPush.sh
    CopyWorkspaceFile "$source_root" gitpush
    CopyWorkspaceFileIfMissing "$source_root" GitHubRepo.address
    CopyWorkspaceFileIfMissing "$source_root" .gitignore
}

RunPostUpdateGitPush()
{
    local gitpush_script="$SdkRoot/Common/Scripts/GitPushOryn.sh"

    if [ "$AutoGitPush" -ne 1 ]; then
        Warn "Automatic GitPush after update is disabled for this run."
        return 0
    fi

    if [ "${ORYN_SKIP_GITPUSH_AFTER_UPDATE:-0}" = "1" ]; then
        Warn "Automatic GitPush after update skipped because ORYN_SKIP_GITPUSH_AFTER_UPDATE=1."
        return 0
    fi

    if [ ! -f "$gitpush_script" ]; then
        Warn "Automatic GitPush skipped because GitPushOryn.sh was not found."
        return 0
    fi

    if ! command -v git >/dev/null 2>&1; then
        Warn "Automatic GitPush skipped because git is not installed in WSL."
        return 0
    fi

    Info "Automatic GitPush after update starting."
    if bash "$gitpush_script" --message "Oryn automatic post-update source sync $SelectedVersion"; then
        Ok "Automatic GitPush after update complete."
    else
        Warn "Automatic GitPush after update failed. The update itself completed."
        Warn "Run ./Oryn.sh gitlogin to configure GitHub login, then run ./Oryn.sh gitpush."
    fi
}

CleanSdkRootForFullUpdate()
{
    local entry
    mkdir -p "$SdkRoot"

    shopt -s dotglob nullglob
    for entry in "$SdkRoot"/*; do
        case "$(basename "$entry")" in
            .git|.gitignore)
                Warn "Preserved $(basename "$entry")"
                ;;
            *)
                rm -rf "$entry"
                ;;
        esac
    done
    shopt -u dotglob nullglob
}

RemoveObsoleteSdkSourcePaths()
{
    local obsolete
    for obsolete in         "$SdkRoot/Source"         "$SdkRoot/Bin"         "$SdkRoot/Docs"         "$SdkRoot/Scripts"         "$SdkRoot/Common/Boot"; do
        if [ -e "$obsolete" ]; then
            rm -rf "$obsolete"
            Ok "Removed obsolete SDK path: ${obsolete#$SdkRoot/}"
        fi
    done
}


RemoveMovedKernelFile()
{
    local moved_path="$1"

    if [ -e "$moved_path" ]; then
        rm -rf "$moved_path"
        Ok "Removed project-owned copy now supplied by SDK: ${moved_path#$WorkspaceRoot/}"
    fi
}

RemoveMovedSharedKernelProjectPaths()
{
    local kernel_project="$ProjectsRoot/Kernel-5"
    [ -d "$kernel_project" ] || return 0

    RemoveMovedKernelFile "$kernel_project/Include/KernelBootInfo.h"
    RemoveMovedKernelFile "$kernel_project/Include/KernelConsole.h"
    RemoveMovedKernelFile "$kernel_project/Include/KernelIo.h"
    RemoveMovedKernelFile "$kernel_project/Include/KernelMemoryMap.h"
    RemoveMovedKernelFile "$kernel_project/Include/KernelPhysicalMemory.h"
    RemoveMovedKernelFile "$kernel_project/Include/KernelTtf.h"
    RemoveMovedKernelFile "$kernel_project/Include/KernelVirtualMemory.h"
    RemoveMovedKernelFile "$kernel_project/Include/Serial.h"

    RemoveMovedKernelFile "$kernel_project/Source/BootInfo"
    RemoveMovedKernelFile "$kernel_project/Source/Console"
    RemoveMovedKernelFile "$kernel_project/Source/Fonts"
    RemoveMovedKernelFile "$kernel_project/Source/KernelIo.c"
    RemoveMovedKernelFile "$kernel_project/Source/Serial.c"
    RemoveMovedKernelFile "$kernel_project/Source/Memory/KernelMemoryMap.c"
    RemoveMovedKernelFile "$kernel_project/Source/Memory/KernelMemoryMapPrint.c"
    RemoveMovedKernelFile "$kernel_project/Source/Memory/KernelPhysicalMemory.c"
    RemoveMovedKernelFile "$kernel_project/Source/Memory/KernelPhysicalMemoryPrint.c"
    RemoveMovedKernelFile "$kernel_project/Source/Memory/KernelVirtualMemory.c"
    RemoveMovedKernelFile "$kernel_project/Source/Memory/KernelVirtualMemoryPrint.c"
    find "$kernel_project/Source" -type d -empty -delete 2>/dev/null || true
    find "$kernel_project/Include" -type d -empty -delete 2>/dev/null || true
}


ApplyChangedFiles()
{
    local extract_root="$1"
    local changed_root="$extract_root/ChangedFiles"

    [ -d "$changed_root" ] || {
        Fail "Archive does not contain ChangedFiles."
        exit 1
    }

    CopyTree "$changed_root/OrynSDK" "$SdkRoot"
    CopyTree "$changed_root/OrynProjects" "$ProjectsRoot"
    CopyWorkspaceShellAndGitFiles "$changed_root"
}

ApplyFullSource()
{
    local extract_root="$1"
    local full_root="$extract_root/FullSource"

    [ -d "$full_root/OrynSDK" ] || {
        Fail "Archive does not contain FullSource/OrynSDK."
        exit 1
    }

    CleanSdkRootForFullUpdate
    CopyTree "$full_root/OrynSDK" "$SdkRoot"
    CopyTree "$full_root/OrynProjects" "$ProjectsRoot"
    CopyWorkspaceShellAndGitFiles "$full_root"
}

ScriptPath="$(CanonicalPath "${BASH_SOURCE[0]}")"
ScriptDir="$(dirname "$ScriptPath")"

if [ -n "${ORYN_WORKSPACE_ROOT:-}" ]; then
    WorkspaceRoot="$(CanonicalPath "$ORYN_WORKSPACE_ROOT")"
    SdkRoot="$(CanonicalPath "${ORYN_SDK_ROOT:-$WorkspaceRoot/OrynSDK}")"
elif [ -n "${ORYN_SDK_ROOT:-}" ]; then
    SdkRoot="$(CanonicalPath "$ORYN_SDK_ROOT")"
    WorkspaceRoot="$(CanonicalPath "$SdkRoot/..")"
elif [ "$(basename "$ScriptDir")" = "Scripts" ] && [ "$(basename "$(dirname "$ScriptDir")")" = "Common" ]; then
    SdkRoot="$(CanonicalPath "$ScriptDir/../..")"
    WorkspaceRoot="$(CanonicalPath "$SdkRoot/..")"
elif [ "$(basename "$ScriptDir")" = "Scripts" ]; then
    SdkRoot="$(CanonicalPath "$ScriptDir/..")"
    WorkspaceRoot="$(CanonicalPath "$SdkRoot/..")"
else
    WorkspaceRoot="$(CanonicalPath "$ScriptDir")"
    SdkRoot="$(CanonicalPath "$WorkspaceRoot/OrynSDK")"
fi

ProjectsRoot="$(CanonicalPath "${ORYN_PROJECTS_ROOT:-$WorkspaceRoot/OrynProjects}")"
Mode="changed"
ZipPath=""
AutoGitPush=1

for arg in "$@"; do
    case "$arg" in
        all|full|--all|--full)
            Mode="full"
            ;;
        changed|update|--changed)
            Mode="changed"
            ;;
        *.zip)
            ZipPath="$arg"
            ;;
        --no-gitpush|--no-git-push|--skip-gitpush)
            AutoGitPush=0
            ;;
        --gitpush|--git-push)
            AutoGitPush=1
            ;;
        --help|-h)
            printf "Oryn WSL updater\n\n"
            printf "Usage:\n"
            printf "  ./update\n"
            printf "  ./update all\n"
            printf "  ./update /path/to/OrynWsl-0.1.2.zip\n"
            printf "  ./update --no-gitpush\n\n"
            printf "Workspace root: %s\n" "$WorkspaceRoot"
            printf "SDK root: %s\n" "$SdkRoot"
            printf "Projects root: %s\n" "$ProjectsRoot"
            exit 0
            ;;
        *)
            Warn "Ignoring unknown argument: $arg"
            ;;
    esac
done

RequireCommand unzip
RequireCommand find
RequireCommand awk
RequireCommand sort
RequireCommand cp

Info "Oryn WSL update"
Info "Workspace root: $WorkspaceRoot"
Info "SDK root: $SdkRoot"
Info "Projects root: $ProjectsRoot"
Info "Mode: $Mode"

TempRoot="$(mktemp -d)"
CandidatesFile="$TempRoot/candidates.txt"
trap 'rm -rf "$TempRoot"' EXIT

if [ -n "$ZipPath" ]; then
    [ -f "$ZipPath" ] || {
        Fail "Archive was not found: $ZipPath"
        exit 1
    }
    ZipPath="$(CanonicalPath "$ZipPath")"
else
    ZipPath="$(FindNewestZip "$CandidatesFile")"
fi

[ -n "$ZipPath" ] || {
    Fail "No OrynWsl-*.zip archive found."
    Warn "Put the zip in ~/Downloads, the current folder, or /mnt/c/Users/<you>/Downloads."
    exit 1
}

Ok "Selected archive: $ZipPath"
SelectedVersion="$(basename "$ZipPath" | sed -n 's/^OrynWsl-\(.*\)\.zip$/\1/p')"
[ -n "$SelectedVersion" ] || SelectedVersion="unknown"
Info "Selected archive version: $SelectedVersion"
unzip -q "$ZipPath" -d "$TempRoot/extract" || {
    Fail "Could not extract archive."
    exit 1
}

if [ "$Mode" = "full" ]; then
    ApplyFullSource "$TempRoot/extract"
else
    ApplyChangedFiles "$TempRoot/extract"
fi

RemoveLegacyGeneratedProjects
RemoveObsoleteSdkSourcePaths
RemoveMovedSharedKernelProjectPaths

chmod +x \
  "$WorkspaceRoot/Oryn.sh" \
  "$WorkspaceRoot/oryn" \
  "$WorkspaceRoot/update" \
  "$WorkspaceRoot/update.sh" \
  "$WorkspaceRoot/GitPush.sh" \
  "$WorkspaceRoot/gitpush" \
  "$SdkRoot/Oryn.sh" \
  "$SdkRoot/oryn" \
  "$SdkRoot/update" \
  "$SdkRoot/update.sh" \
  "$SdkRoot/GitPush.sh" \
  "$SdkRoot/gitpush" \
  "$SdkRoot/Common/Scripts/BuildOryn.sh" \
  "$SdkRoot/Common/Scripts/UpdateOryn.sh" \
  "$SdkRoot/Common/Scripts/UpdateOrynCurrent.sh" 2>/dev/null || true

Ok "Update complete."
RunPostUpdateGitPush
Info "Run from workspace root: cd $WorkspaceRoot && ./Oryn.sh"
Info "Run from SDK root: cd $SdkRoot && ./Oryn.sh"
