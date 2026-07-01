#!/usr/bin/env bash
set -euo pipefail

PackageZip="${1:-}"
RootArg="${2:-$(pwd)}"

Info(){ printf '[INFO] %s\n' "$*"; }
Ok(){ printf '[ OK ] %s\n' "$*"; }
Fail(){ printf '[FAIL] %s\n' "$*" >&2; exit 1; }

if [ -z "$PackageZip" ]; then
    Fail "usage: bash OrynReleaseKeyRecovery-0.5.90-fixed.sh <OrynWsl-package.zip> [workspace-or-sdk-root]"
fi

if [ ! -f "$PackageZip" ]; then
    Fail "package zip not found: $PackageZip"
fi

if [ ! -d "$RootArg" ]; then
    Fail "root not found: $RootArg"
fi

if ! command -v unzip >/dev/null 2>&1; then
    Fail "unzip is required."
fi

if ! command -v openssl >/dev/null 2>&1; then
    Fail "openssl is required."
fi

# Accept either:
#   workspace root: ~/OrynSDK              -> SDK root is ~/OrynSDK/OrynSDK
#   actual SDK root: ~/OrynSDK/OrynSDK     -> SDK root is itself
if [ -d "$RootArg/OrynSDK/Common/Security" ] || [ -d "$RootArg/OrynSDK" -a -f "$RootArg/update.sh" ]; then
    WorkspaceRoot="$RootArg"
    SdkRoot="$RootArg/OrynSDK"
elif [ -d "$RootArg/Common" ] || [ -f "$RootArg/Oryn.sh" ]; then
    SdkRoot="$RootArg"
    WorkspaceRoot="$(dirname "$RootArg")"
else
    Fail "could not detect SDK root. Pass either ~/OrynSDK or ~/OrynSDK/OrynSDK."
fi

Info "Workspace root: $WorkspaceRoot"
Info "SDK root: $SdkRoot"

TempRoot="$(mktemp -d)"
trap 'rm -rf "$TempRoot"' EXIT

HashesPath="$TempRoot/PackageHashes.txt"
SigPath="$TempRoot/PackageHashes.sig"
PkgKeyPath="$TempRoot/OrynReleasePublicKey.pem"
InstalledKeyPath="$SdkRoot/Common/Security/OrynReleasePublicKey.pem"
BackupPath="$InstalledKeyPath.pre-0.5.90-key-rotation.bak"

Info "Extracting release authenticity files from package."
unzip -p "$PackageZip" PackageHashes.txt > "$HashesPath" || Fail "PackageHashes.txt missing from package."
unzip -p "$PackageZip" PackageHashes.sig > "$SigPath" || Fail "PackageHashes.sig missing from package."
unzip -p "$PackageZip" FullSource/OrynSDK/Common/Security/OrynReleasePublicKey.pem > "$PkgKeyPath" || Fail "package release public key missing."

Info "Verifying package hash signature with packaged public key."
openssl dgst -sha256 -verify "$PkgKeyPath" -signature "$SigPath" "$HashesPath" >/dev/null 2>&1 || Fail "package signature does not verify with packaged key."
Ok "PackageHashes.txt signature verifies with packaged release key."

DeclaredKeyId="$(sed -n 's/^PublicKeyId=//p' "$HashesPath" | head -n 1 | tr -d '\r')"
ActualKeyId="$(openssl pkey -pubin -in "$PkgKeyPath" -outform DER 2>/dev/null | sha256sum | awk '{print $1}')"

if [ -z "$DeclaredKeyId" ]; then
    Fail "PackageHashes.txt does not declare PublicKeyId."
fi

if [ "$DeclaredKeyId" != "$ActualKeyId" ]; then
    Fail "packaged key id mismatch. Declared=$DeclaredKeyId Actual=$ActualKeyId"
fi
Ok "Package release public key id verified: $ActualKeyId"

DeclaredVersion="$(sed -n 's/^PackageVersion=//p' "$HashesPath" | head -n 1 | tr -d '\r')"
Ok "Package version declared by signed hash manifest: $DeclaredVersion"

mkdir -p "$(dirname "$InstalledKeyPath")"

if [ -f "$InstalledKeyPath" ]; then
    OldKeyId="$(openssl pkey -pubin -in "$InstalledKeyPath" -outform DER 2>/dev/null | sha256sum | awk '{print $1}' || true)"
    Info "Currently installed key id: ${OldKeyId:-unreadable}"
    if cmp -s "$InstalledKeyPath" "$PkgKeyPath"; then
        Ok "Installed release public key already matches package key."
    else
        cp "$InstalledKeyPath" "$BackupPath"
        Ok "Backed up previous installed release public key to: $BackupPath"
        cp "$PkgKeyPath" "$InstalledKeyPath"
        Ok "Installed package release public key to: $InstalledKeyPath"
    fi
else
    cp "$PkgKeyPath" "$InstalledKeyPath"
    Ok "Installed first release public key to: $InstalledKeyPath"
fi

NewKeyId="$(openssl pkey -pubin -in "$InstalledKeyPath" -outform DER 2>/dev/null | sha256sum | awk '{print $1}')"
if [ "$NewKeyId" != "$ActualKeyId" ]; then
    Fail "installed key still does not match package key after copy. Installed=$NewKeyId Package=$ActualKeyId"
fi
Ok "Installed trusted key now matches the package key."

Info "Testing the exact verification command that the updater uses."
openssl dgst -sha256 -verify "$InstalledKeyPath" -signature "$SigPath" "$HashesPath" >/dev/null 2>&1 || Fail "signature still fails with installed key."
Ok "PackageHashes.txt signature verifies with installed trusted key."

Ok "Release key recovery complete. Now rerun the normal SDK update using the same package."
