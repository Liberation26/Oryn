#!/usr/bin/env bash
set -euo pipefail

SDK_ROOT="${1:-/home/dave/OrynSDK}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PATCH_ROOT="$SCRIPT_DIR/ChangedFiles"
SOURCE="$PATCH_ROOT/OrynProjects/Kernel-5/Source/KernelMain.c"
TARGET="$SDK_ROOT/OrynProjects/Kernel-5/Source/KernelMain.c"

if [ ! -f "$SOURCE" ]; then
    echo "[FAIL] Patch file missing: $SOURCE"
    exit 1
fi

mkdir -p "$(dirname "$TARGET")"

if [ -f "$TARGET" ]; then
    cp "$TARGET" "$TARGET.before-public-return-rule-0.0.3"
    echo "[ OK ] Backed up existing KernelMain.c to $TARGET.before-public-return-rule-0.0.3"
fi

cp "$SOURCE" "$TARGET"
echo "[ OK ] Updated KernelMain.c: $TARGET"

if [ -f "$PATCH_ROOT/Common/Kernel/Include/OrynStatus.h" ]; then
    mkdir -p "$SDK_ROOT/Common/Kernel/Include"
    cp "$PATCH_ROOT/Common/Kernel/Include/OrynStatus.h" "$SDK_ROOT/Common/Kernel/Include/OrynStatus.h"
    echo "[ OK ] Installed OrynStatus.h"
fi

if [ -f "$PATCH_ROOT/Common/Kernel/Source/Status/OrynStatus.c" ]; then
    mkdir -p "$SDK_ROOT/Common/Kernel/Source/Status"
    cp "$PATCH_ROOT/Common/Kernel/Source/Status/OrynStatus.c" "$SDK_ROOT/Common/Kernel/Source/Status/OrynStatus.c"
    echo "[ OK ] Installed OrynStatus.c"
fi

echo "[STEP] KernelMain now returns OrynStatus and checks OrynKernelSdkWriteLine."
echo "[STEP] If the build now fails, the next file to fix is OrynKernelSdkWriteLine/ORYN_KERNEL_APPLICATION so they also use OrynStatus."
