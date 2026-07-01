#!/usr/bin/env bash
set -u

Info() { printf '[INFO] %s\n' "$1"; }
Ok() { printf '[ OK ] %s\n' "$1"; }
Warn() { printf '[WARN] %s\n' "$1"; }
Fail() { printf '[FAIL] %s\n' "$1" >&2; }

if [ "$#" -lt 1 ]; then
    Fail "Usage: bash OrynNoValidationBootstrap-0.5.111.sh <OrynWsl-0.5.111 zip> [workspace-root]"
    exit 1
fi

ZipPath="$1"
WorkspaceRoot="${2:-$HOME/OrynSDK}"
SdkRoot="$WorkspaceRoot/OrynSDK"

if [ ! -f "$ZipPath" ]; then
    Fail "Zip not found: $ZipPath"
    exit 1
fi

if [ ! -d "$WorkspaceRoot" ]; then
    Fail "Workspace root not found: $WorkspaceRoot"
    exit 1
fi

TempRoot="$(mktemp -d)"
trap 'rm -rf "$TempRoot"' EXIT

Info "Applying no-validation updater bootstrap."
Info "Archive: $ZipPath"
Info "Workspace root: $WorkspaceRoot"
Info "SDK root: $SdkRoot"
Warn "No package validation or signature verification will be performed."

unzip -q "$ZipPath" -d "$TempRoot/extract" || {
    Fail "Could not extract archive."
    exit 1
}

if [ ! -d "$TempRoot/extract/ChangedFiles" ]; then
    Fail "ChangedFiles folder missing in archive."
    exit 1
fi

cp -a "$TempRoot/extract/ChangedFiles/." "$WorkspaceRoot/"

if [ -f "$WorkspaceRoot/OrynSDK/Common/Scripts/UpdateOrynCurrent.sh" ]; then
    chmod +x "$WorkspaceRoot/OrynSDK/Common/Scripts/UpdateOrynCurrent.sh"
fi

for path in \
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
    "$SdkRoot/Common/Scripts/UpdateOrynCurrent.sh"
do
    [ -e "$path" ] && chmod +x "$path"
done

Ok "Installed no-validation updater files from ChangedFiles."
Ok "Now run: cd $WorkspaceRoot && ./update.sh"
