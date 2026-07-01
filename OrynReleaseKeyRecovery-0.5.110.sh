#!/usr/bin/env bash
set -euo pipefail

PACKAGE_PATH="${1:-}"
WORKSPACE_OR_SDK_ROOT="${2:-}"

fail() { printf '[FAIL] %s\n' "$1"; exit 1; }
ok() { printf '[ OK ] %s\n' "$1"; }
info() { printf '[INFO] %s\n' "$1"; }

if [ -z "$PACKAGE_PATH" ] || [ -z "$WORKSPACE_OR_SDK_ROOT" ]; then
    fail "Usage: bash ./OrynReleaseKeyRecovery-0.5.110.sh <package.zip> <workspace-or-sdk-root>"
fi

[ -f "$PACKAGE_PATH" ] || fail "Package not found: $PACKAGE_PATH"
[ -d "$WORKSPACE_OR_SDK_ROOT" ] || fail "Root directory not found: $WORKSPACE_OR_SDK_ROOT"

ROOT="$WORKSPACE_OR_SDK_ROOT"
if [ -d "$ROOT/OrynSDK/Common/Security" ]; then
    SDK_ROOT="$ROOT/OrynSDK"
elif [ -d "$ROOT/Common/Security" ]; then
    SDK_ROOT="$ROOT"
else
    fail "Could not find SDK Common/Security under $ROOT"
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
unzip -q "$PACKAGE_PATH" -d "$TMP"

PACKAGE_KEY="$TMP/FullSource/OrynSDK/Common/Security/OrynReleasePublicKey.pem"
PACKAGE_HASHES="$TMP/PackageHashes.txt"
PACKAGE_SIG="$TMP/PackageHashes.sig"
[ -f "$PACKAGE_KEY" ] || fail "Package public key missing."
[ -f "$PACKAGE_HASHES" ] || fail "PackageHashes.txt missing."
[ -f "$PACKAGE_SIG" ] || fail "PackageHashes.sig missing."

openssl dgst -sha256 -verify "$PACKAGE_KEY" -signature "$PACKAGE_SIG" "$PACKAGE_HASHES" >/dev/null || fail "Package signature does not verify with packaged key."
ok "Package signature verifies with packaged key."

EXPECTED_KEY_ID="$(grep '^PublicKeyId=' "$PACKAGE_HASHES" | head -n1 | cut -d= -f2-)"
ACTUAL_KEY_ID="$(sha256sum "$PACKAGE_KEY" | awk '{print $1}')"
[ "$EXPECTED_KEY_ID" = "$ACTUAL_KEY_ID" ] || fail "Packaged key id mismatch."
ok "Packaged key id verified: $ACTUAL_KEY_ID"

INSTALLED_KEY="$SDK_ROOT/Common/Security/OrynReleasePublicKey.pem"
[ -d "$(dirname "$INSTALLED_KEY")" ] || mkdir -p "$(dirname "$INSTALLED_KEY")"
if [ -f "$INSTALLED_KEY" ]; then
    BACKUP="$INSTALLED_KEY.backup-before-0.5.110.$(date +%Y%m%d%H%M%S)"
    cp "$INSTALLED_KEY" "$BACKUP"
    ok "Backed up previous release key: $BACKUP"
fi
cp "$PACKAGE_KEY" "$INSTALLED_KEY"
ok "Installed release key to: $INSTALLED_KEY"

openssl dgst -sha256 -verify "$INSTALLED_KEY" -signature "$PACKAGE_SIG" "$PACKAGE_HASHES" >/dev/null || fail "Installed key still cannot verify package signature."
ok "Installed key verifies this package."
info "Now rerun the normal updater from the workspace root: ./update.sh"
