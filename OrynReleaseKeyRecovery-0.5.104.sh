#!/usr/bin/env bash
set -euo pipefail

PACKAGE="${1:-}"
WORKSPACE_OR_SDK="${2:-$HOME/OrynSDK}"
if [ -z "$PACKAGE" ]; then
  echo "[FAIL] Usage: bash ./OrynReleaseKeyRecovery-0.5.104.sh <OrynWsl-0.5.104 zip> [workspace-or-sdk-root]" >&2
  exit 1
fi
if [ ! -f "$PACKAGE" ]; then
  echo "[FAIL] Package not found: $PACKAGE" >&2
  exit 1
fi
ROOT="$WORKSPACE_OR_SDK"
if [ -d "$ROOT/OrynSDK/Common/Security" ]; then
  SDK_ROOT="$ROOT/OrynSDK"
elif [ -d "$ROOT/Common/Security" ] || [ -f "$ROOT/VERSION" ]; then
  SDK_ROOT="$ROOT"
else
  echo "[FAIL] Could not find SDK root under: $ROOT" >&2
  echo "[INFO] Pass either ~/OrynSDK or ~/OrynSDK/OrynSDK" >&2
  exit 1
fi
TMP="$(mktemp -d)"
cleanup(){ rm -rf "$TMP"; }
trap cleanup EXIT
unzip -q "$PACKAGE" PackageHashes.txt PackageHashes.sig FullSource/OrynSDK/Common/Security/OrynReleasePublicKey.pem -d "$TMP"
PUB="$TMP/FullSource/OrynSDK/Common/Security/OrynReleasePublicKey.pem"
openssl dgst -sha256 -verify "$PUB" -signature "$TMP/PackageHashes.sig" "$TMP/PackageHashes.txt" >/dev/null
KEY_ID="$(openssl pkey -pubin -in "$PUB" -outform DER | sha256sum | awk '{print $1}')"
DECLARED="$(awk -F= '/^PublicKeyId=/{print $2}' "$TMP/PackageHashes.txt")"
if [ "$KEY_ID" != "$DECLARED" ]; then
  echo "[FAIL] Public key ID mismatch." >&2
  echo "[INFO] Declared: $DECLARED" >&2
  echo "[INFO] Actual  : $KEY_ID" >&2
  exit 1
fi
mkdir -p "$SDK_ROOT/Common/Security"
if [ -f "$SDK_ROOT/Common/Security/OrynReleasePublicKey.pem" ]; then
  cp "$SDK_ROOT/Common/Security/OrynReleasePublicKey.pem" "$SDK_ROOT/Common/Security/OrynReleasePublicKey.pem.before-0.5.104"
fi
cp "$PUB" "$SDK_ROOT/Common/Security/OrynReleasePublicKey.pem"
openssl dgst -sha256 -verify "$SDK_ROOT/Common/Security/OrynReleasePublicKey.pem" -signature "$TMP/PackageHashes.sig" "$TMP/PackageHashes.txt" >/dev/null
echo "[ OK ] Installed trusted Oryn release key for package 0.5.104."
echo "[ OK ] Key ID: $KEY_ID"
echo "[INFO] Now rerun: cd ${SDK_ROOT%/OrynSDK} && ./update.sh"
