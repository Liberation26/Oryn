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


AppendValidationReport()
{
    local level="$1"
    local message="$2"

    [ -n "${PackageValidationReport:-}" ] || return 0
    printf '[%s] %s\n' "$level" "$message" >> "$PackageValidationReport"
}

ValidationOk()
{
    AppendValidationReport " OK " "$1"
    Ok "Package validation: $1"
}

ValidationInfo()
{
    AppendValidationReport "INFO" "$1"
    Info "Package validation: $1"
}

ValidationWarn()
{
    AppendValidationReport "WARN" "$1"
    Warn "Package validation: $1"
}

ValidationFail()
{
    AppendValidationReport "FAIL" "$1"
    Fail "Package validation: $1"
    PackageValidationFailed=1
}

NormaliseVersionText()
{
    tr -d '\r\n\t ' < "$1"
}

SelectedPackageBaseVersion()
{
    local selected="$1"
    printf '%s\n' "${selected%%-*}"
}

PackagePathIsUnsafe()
{
    local path="$1"

    case "$path" in
        ""|/*|~*|*'\\'*|*'//'*)
            return 0
            ;;
        *../*|../*|*/..|*'/../'*|*'/.'|./*)
            return 0
            ;;
    esac

    return 1
}

ValidateZipEntryList()
{
    local zip_path="$1"
    local entries_file="$2"
    local entry
    local count=0

    : > "$entries_file"
    if ! unzip -Z1 "$zip_path" > "$entries_file" 2>/dev/null; then
        ValidationFail "could not read zip entry list before extraction."
        return 0
    fi

    while IFS= read -r entry || [ -n "$entry" ]; do
        entry="${entry%$'\r'}"
        [ -n "$entry" ] || continue
        count=$((count + 1))
        if PackagePathIsUnsafe "$entry"; then
            ValidationFail "unsafe archive path rejected before extraction: $entry"
        fi
    done < "$entries_file"

    if [ "$count" -eq 0 ]; then
        ValidationFail "archive has no entries."
    else
        ValidationOk "archive entry list is readable and contains $count entries."
    fi
}

RequirePackageFile()
{
    local root="$1"
    local relative="$2"

    if [ -f "$root/$relative" ]; then
        ValidationOk "required file present: $relative"
    else
        ValidationFail "required file missing: $relative"
    fi
}

RequirePackageDir()
{
    local root="$1"
    local relative="$2"

    if [ -d "$root/$relative" ]; then
        ValidationOk "required directory present: $relative"
    else
        ValidationFail "required directory missing: $relative"
    fi
}

ValidateNoExtractedSymlinks()
{
    local extract_root="$1"
    local found

    found="$(find "$extract_root" -type l -print -quit 2>/dev/null)"
    if [ -n "$found" ]; then
        ValidationFail "package contains symlink entry: ${found#$extract_root/}"
    else
        ValidationOk "package contains no extracted symlinks."
    fi
}

ValidateExtractedPathSafety()
{
    local extract_root="$1"
    local relative
    local failed=0

    while IFS= read -r relative || [ -n "$relative" ]; do
        relative="${relative#./}"
        [ -n "$relative" ] || continue
        if PackagePathIsUnsafe "$relative"; then
            ValidationFail "unsafe extracted path rejected: $relative"
            failed=1
        fi
    done < <(cd "$extract_root" && find . -mindepth 1 -print 2>/dev/null)

    if [ "$failed" -eq 0 ]; then
        ValidationOk "all extracted paths are relative and safe."
    fi
}

ValidateNoNestedPackageRoots()
{
    local extract_root="$1"
    local nested

    nested="$(find "$extract_root/ChangedFiles" "$extract_root/FullSource" -mindepth 2 \( -path '*/ChangedFiles/FullSource' -o -path '*/ChangedFiles/ChangedFiles' -o -path '*/FullSource/ChangedFiles' -o -path '*/FullSource/FullSource' \) -print -quit 2>/dev/null)"
    if [ -n "$nested" ]; then
        ValidationFail "package contains nested package root: ${nested#$extract_root/}"
    else
        ValidationOk "package roots are not nested inside payload folders."
    fi
}

ValidateDeletedFilesManifest()
{
    local extract_root="$1"
    local deleted_file="$extract_root/ChangedFiles/DeletedFiles.txt"
    local relative_path
    local count=0

    if [ ! -f "$deleted_file" ]; then
        ValidationInfo "DeletedFiles.txt not present; no delete operations requested."
        return 0
    fi

    while IFS= read -r relative_path || [ -n "$relative_path" ]; do
        relative_path="${relative_path%$'\r'}"
        case "$relative_path" in
            ""|\#*)
                continue
                ;;
        esac
        count=$((count + 1))
        if PackagePathIsUnsafe "$relative_path"; then
            ValidationFail "unsafe DeletedFiles.txt entry rejected: $relative_path"
        fi
    done < "$deleted_file"

    ValidationOk "DeletedFiles.txt checked with $count delete entr$( [ "$count" -eq 1 ] && printf 'y' || printf 'ies' )."
}

ValidateVersionFiles()
{
    local extract_root="$1"
    local selected_version="$2"
    local expected
    local full_version
    local changed_version

    expected="$(SelectedPackageBaseVersion "$selected_version")"
    full_version="$(NormaliseVersionText "$extract_root/FullSource/OrynSDK/VERSION" 2>/dev/null || true)"
    changed_version="$(NormaliseVersionText "$extract_root/ChangedFiles/OrynSDK/VERSION" 2>/dev/null || true)"

    if [ -z "$full_version" ]; then
        ValidationFail "FullSource/OrynSDK/VERSION is empty."
    fi

    if [ -z "$changed_version" ]; then
        ValidationFail "ChangedFiles/OrynSDK/VERSION is empty."
    fi

    if [ -n "$full_version" ] && [ -n "$changed_version" ] && [ "$full_version" != "$changed_version" ]; then
        ValidationFail "FullSource and ChangedFiles VERSION values differ: $full_version vs $changed_version"
    elif [ -n "$full_version" ]; then
        ValidationOk "FullSource and ChangedFiles VERSION agree: $full_version"
    fi

    if [ -n "$expected" ] && [ -n "$full_version" ] && [ "$expected" != "$full_version" ]; then
        ValidationFail "archive name version $expected does not match package VERSION $full_version."
    elif [ -n "$expected" ] && [ -n "$full_version" ]; then
        ValidationOk "archive name version matches package VERSION: $expected"
    fi
}

ValidateModePayload()
{
    local extract_root="$1"
    local mode="$2"

    if [ "$mode" = "full" ]; then
        RequirePackageDir "$extract_root" "FullSource/OrynSDK"
        RequirePackageFile "$extract_root" "FullSource/OrynSDK/VERSION"
        RequirePackageFile "$extract_root" "FullSource/OrynSDK/Common/Scripts/UpdateOrynCurrent.sh"
    else
        RequirePackageDir "$extract_root" "ChangedFiles"
        RequirePackageDir "$extract_root" "ChangedFiles/OrynSDK"
        RequirePackageFile "$extract_root" "ChangedFiles/OrynSDK/VERSION"
    fi
}

ValidateCriticalSdkFiles()
{
    local extract_root="$1"

    RequirePackageDir "$extract_root" "FullSource"
    RequirePackageDir "$extract_root" "ChangedFiles"
    RequirePackageDir "$extract_root" "FullSource/OrynSDK/Common/Scripts"
    RequirePackageDir "$extract_root" "FullSource/OrynSDK/Common/OrynBuild"
    RequirePackageFile "$extract_root" "FullSource/OrynSDK/VERSION"
    RequirePackageFile "$extract_root" "ChangedFiles/OrynSDK/VERSION"
    RequirePackageFile "$extract_root" "FullSource/OrynSDK/Oryn.sh"
    RequirePackageFile "$extract_root" "FullSource/OrynSDK/oryn"
    RequirePackageFile "$extract_root" "FullSource/OrynSDK/update"
    RequirePackageFile "$extract_root" "FullSource/OrynSDK/update.sh"
    RequirePackageFile "$extract_root" "FullSource/OrynSDK/Common/Scripts/UpdateOryn.sh"
    RequirePackageFile "$extract_root" "FullSource/OrynSDK/Common/Scripts/UpdateOrynCurrent.sh"
    RequirePackageFile "$extract_root" "FullSource/OrynSDK/Common/Scripts/BuildOryn.sh"
    RequirePackageFile "$extract_root" "FullSource/OrynSDK/Common/OrynBuild/Main.c"
    RequirePackageFile "$extract_root" "FullSource/OrynSDK/Common/OrynBuild/OrynBuild.h"
}

ValidatePackageBeforeCopy()
{
    local zip_path="$1"
    local extract_root="$2"
    local mode="$3"
    local selected_version="$4"
    local entries_file="$5"

    PackageValidationFailed=0
    PackageValidationReport="$TempRoot/PackageSelfValidationReport.txt"
    : > "$PackageValidationReport"

    ValidationInfo "archive: $zip_path"
    ValidationInfo "selected version: $selected_version"
    ValidationInfo "mode: $mode"
    ValidationInfo "workspace root: $WorkspaceRoot"
    ValidationInfo "SDK root: $SdkRoot"
    ValidationInfo "projects root: $ProjectsRoot"

    ValidateZipEntryList "$zip_path" "$entries_file"
    ValidateNoExtractedSymlinks "$extract_root"
    ValidateExtractedPathSafety "$extract_root"
    ValidateCriticalSdkFiles "$extract_root"
    ValidateModePayload "$extract_root" "$mode"
    ValidateVersionFiles "$extract_root" "$selected_version"
    ValidateNoNestedPackageRoots "$extract_root"
    ValidateDeletedFilesManifest "$extract_root"

    if [ "$PackageValidationFailed" -ne 0 ]; then
        Fail "Package self-validation failed. No files were copied."
        Fail "Validation report: $PackageValidationReport"
        exit 1
    fi

    Ok "Package self-validation passed before any update copy."
    Info "Validation report: $PackageValidationReport"
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


ApplyDeletedFiles()
{
    local extract_root="$1"
    local deleted_file="$extract_root/ChangedFiles/DeletedFiles.txt"
    local relative_path
    local target_path

    [ -f "$deleted_file" ] || return 0

    while IFS= read -r relative_path || [ -n "$relative_path" ]; do
        relative_path="${relative_path%$'\r'}"
        case "$relative_path" in
            ""|\#*)
                continue
                ;;
            /*|*..*)
                Warn "Ignoring unsafe deleted-file entry: $relative_path"
                continue
                ;;
        esac

        target_path="$WorkspaceRoot/$relative_path"
        if [ -e "$target_path" ]; then
            rm -rf "$target_path"
            Ok "Removed obsolete file: $relative_path"
        fi
    done < "$deleted_file"
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

ValidatePackageBeforeCopy "$ZipPath" "$TempRoot/extract" "$Mode" "$SelectedVersion" "$TempRoot/ZipEntries.txt"

if [ "$Mode" = "full" ]; then
    ApplyFullSource "$TempRoot/extract"
else
    ApplyChangedFiles "$TempRoot/extract"
fi

RemoveLegacyGeneratedProjects
RemoveObsoleteSdkSourcePaths
RemoveMovedSharedKernelProjectPaths
ApplyDeletedFiles "$TempRoot/extract"

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
