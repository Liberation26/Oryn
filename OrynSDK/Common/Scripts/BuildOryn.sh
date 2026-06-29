#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

mkdir -p "${SDK_ROOT}/Common/Bin"

if ! command -v clang >/dev/null 2>&1; then
    echo "[FAIL] clang was not found in WSL." >&2
    echo "[INFO] Install with: sudo apt install -y clang lld llvm" >&2
    exit 1
fi

clang \
  -std=c11 \
  -D_DEFAULT_SOURCE \
  -Wall \
  -Wextra \
  -O2 \
  "${SDK_ROOT}"/Common/OrynBuild/*.c \
  "${SDK_ROOT}"/Targets/UEFI/X64/OrynBuild/*.c \
  -I"${SDK_ROOT}/Common/OrynBuild" \
  -o "${SDK_ROOT}/Common/Bin/oryn"

chmod +x "${SDK_ROOT}/Common/Bin/oryn"
echo "[ OK ] Built ${SDK_ROOT}/Common/Bin/oryn"
