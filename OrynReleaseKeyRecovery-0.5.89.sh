#!/usr/bin/env bash
set -euo pipefail

PackageZip="${1:-}"
SdkRoot="${2:-$(pwd)}"

Info(){ printf '[INFO] %s\n' "$*"; }
Ok(){ printf '[ OK ] %s\n' "$*"; }
Fail(){ printf '[FAIL] %s\n' "$*" >&2; exit 1; }

if [ -z "$PackageZip" ]; then
    Fail "usage: bash OrynReleaseKeyRecovery-0.5.89.sh <OrynWsl-package.zip> [OrynSDK-root]"
fi

if [ ! -f "$PackageZip" ]; then
    Fail "package zip not found: $PackageZip"
fi

if [ ! -d "$SdkRoot" ]; then
    Fail "SDK root not found: $SdkRoot"
fi

if ! command -v unzip >/dev/null 2>&1; then
    Fail "unzip is required."
fi

if ! command -v openssl >/dev/null 2>&1; then
    Fail "openssl is required."
fi

TempRoot="$(mktemp -d)"
trap 'rm -rf "$TempRoot"' EXIT

HashesPath="$TempRoot/PackageHashes.txt"
SigPath="$TempRoot/PackageHashes.sig"
PkgKeyPath="$TempRoot/OrynReleasePublicKey.pem"
InstalledKeyPath="$SdkRoot/Common/Security/OrynReleasePublicKey.pem"
BackupPath="$InstalledKeyPath.pre-0.5.89-key-rotation.bak"

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

Ok "Release key recovery complete. Now rerun the normal SDK update using the same package."
