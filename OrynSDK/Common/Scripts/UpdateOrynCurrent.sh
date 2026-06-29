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
    CopyWorkspaceFile "$changed_root" Oryn.sh
    CopyWorkspaceFile "$changed_root" oryn
    CopyWorkspaceFile "$changed_root" update
    CopyWorkspaceFile "$changed_root" update.sh
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
    CopyWorkspaceFile "$full_root" Oryn.sh
    CopyWorkspaceFile "$full_root" oryn
    CopyWorkspaceFile "$full_root" update
    CopyWorkspaceFile "$full_root" update.sh
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
        --help|-h)
            printf "Oryn WSL updater\n\n"
            printf "Usage:\n"
            printf "  ./update\n"
            printf "  ./update all\n"
            printf "  ./update /path/to/OrynWsl-0.1.2.zip\n\n"
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

chmod +x \
  "$WorkspaceRoot/Oryn.sh" \
  "$WorkspaceRoot/oryn" \
  "$WorkspaceRoot/update" \
  "$WorkspaceRoot/update.sh" \
  "$SdkRoot/Oryn.sh" \
  "$SdkRoot/oryn" \
  "$SdkRoot/update" \
  "$SdkRoot/update.sh" \
  "$SdkRoot/Common/Scripts/BuildOryn.sh" \
  "$SdkRoot/Common/Scripts/UpdateOryn.sh" \
  "$SdkRoot/Common/Scripts/UpdateOrynCurrent.sh" 2>/dev/null || true

Ok "Update complete."
Info "Run from workspace root: cd $WorkspaceRoot && ./Oryn.sh"
Info "Run from SDK root: cd $SdkRoot && ./Oryn.sh"
