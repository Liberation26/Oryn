#!/usr/bin/env bash
set -euo pipefail
shopt -s globstar nullglob

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
LIBC_ROOT="${SDK_ROOT}/Common/OrynLibC"
REPORT_DIR="${SDK_ROOT}/Common/Reports/LibCConformance"
REPORT="${REPORT_DIR}/LibCHostConformanceReport.txt"
BUILD_DIR="${REPORT_DIR}/Build"
EXE="${BUILD_DIR}/OrynLibCHostConformance"
MANIFEST_REPORT="${REPORT_DIR}/LibCFunctionManifestCoverage.txt"

ColorReset="\033[0m"
ColorOk="\033[32m"
ColorInfo="\033[36m"
ColorFail="\033[31m"
UseColor=1
[ -n "${NO_COLOR:-}" ] && UseColor=0
[ "${ORYN_NO_COLOR:-0}" = "1" ] && UseColor=0

PrintTag()
{
    local color="$1"
    local tag="$2"
    local message="$3"
    if [ "$UseColor" -eq 1 ]; then
        printf "%b%s%b %s\n" "$color" "$tag" "$ColorReset" "$message"
    else
        printf "%s %s\n" "$tag" "$message"
    fi
}

Info() { PrintTag "$ColorInfo" "[INFO]" "$1"; }
Ok() { PrintTag "$ColorOk" "[ OK ]" "$1"; }
Fail() { PrintTag "$ColorFail" "[FAIL]" "$1"; }

mkdir -p "$BUILD_DIR"
: > "$REPORT"
: > "$MANIFEST_REPORT"

LogReport()
{
    printf "%s\n" "$1" >> "$REPORT"
}

LogManifest()
{
    printf "%s\n" "$1" >> "$MANIFEST_REPORT"
}

if ! command -v clang >/dev/null 2>&1; then
    Fail "clang was not found; cannot run host-side LibC conformance tests."
    exit 1
fi

Info "OrynLibC host conformance test root: ${LIBC_ROOT}"
LogReport "OrynLibC host conformance report"
LogReport "SDK root: ${SDK_ROOT}"
LogReport "LibC root: ${LIBC_ROOT}"
LogReport "Compiler: $(command -v clang)"

mapfile -t sources < <(find "$LIBC_ROOT/Source" -type f -name '*.c' ! -path '*/proofs/*' | sort)
mapfile -t manifests < <(find "$LIBC_ROOT/FunctionManifests" -type f -name '*.libcunit' | sort)

source_count=${#sources[@]}
manifest_count=${#manifests[@]}

LogReport "Source units: ${source_count}"
LogReport "Function-unit manifests: ${manifest_count}"
LogManifest "OrynLibC function-unit manifest coverage"
LogManifest "Source units: ${source_count}"
LogManifest "Function-unit manifests: ${manifest_count}"

if [ "$source_count" -eq 0 ]; then
    Fail "No OrynLibC source units were found."
    exit 1
fi

if [ "$manifest_count" -eq 0 ]; then
    Fail "No OrynLibC function-unit manifests were found."
    exit 1
fi

coverage_failed=0
for source in "${sources[@]}"; do
    rel="${source#${LIBC_ROOT}/Source/}"
    if ! awk -F= -v rel="$rel" '/^Source=/{gsub(/\r/, "", $2); if ($2 == rel) found=1} END{exit found ? 0 : 1}' "$LIBC_ROOT"/FunctionManifests/**/*.libcunit 2>/dev/null; then
        LogManifest "MISSING-MANIFEST ${rel}"
        coverage_failed=1
    else
        LogManifest "SOURCE-COVERED ${rel}"
    fi
done

for manifest in "${manifests[@]}"; do
    rel_source="$(awk -F= '/^Source=/{gsub(/\r/, "", $2); print $2; exit}' "$manifest")"
    public_symbols="$(awk -F= '/^PublicSymbols=/{gsub(/\r/, "", $2); print $2; exit}' "$manifest")"
    if [[ "$rel_source" == proofs/* ]]; then
        LogManifest "KERNEL-PROOF-MANIFEST ${manifest#${LIBC_ROOT}/FunctionManifests/}"
        continue
    fi
    if [ -z "$rel_source" ]; then
        LogManifest "INVALID-MANIFEST no Source field: ${manifest#${LIBC_ROOT}/FunctionManifests/}"
        coverage_failed=1
        continue
    fi
    if [ ! -f "$LIBC_ROOT/Source/$rel_source" ]; then
        LogManifest "MISSING-SOURCE ${rel_source} from ${manifest#${LIBC_ROOT}/FunctionManifests/}"
        coverage_failed=1
    fi
    if [ -z "$public_symbols" ] || [ "$public_symbols" = "-" ]; then
        LogManifest "MISSING-PUBLIC-SYMBOLS ${manifest#${LIBC_ROOT}/FunctionManifests/}"
        coverage_failed=1
    else
        LogManifest "PUBLIC-SYMBOLS ${manifest#${LIBC_ROOT}/FunctionManifests/}: ${public_symbols}"
    fi
done

if [ "$coverage_failed" -ne 0 ]; then
    Fail "OrynLibC function-unit manifest coverage failed."
    Info "Coverage report: ${MANIFEST_REPORT}"
    exit 1
fi

Info "Compiling host-side OrynLibC conformance executable."
clang \
    -std=c11 \
    -D_DEFAULT_SOURCE \
    -Wall \
    -Wextra \
    -Werror \
    -fno-builtin \
    -I"$LIBC_ROOT/Include" \
    -I"$LIBC_ROOT/Source/stdlib" \
    "${sources[@]}" \
    "$LIBC_ROOT/Tests/Host/TestOrynLibCHostConformance.c" \
    -o "$EXE"

Info "Checking public symbol inventory with nm."
nm "$EXE" > "$BUILD_DIR/OrynLibCHostConformance.nm"
for manifest in "${manifests[@]}"; do
    rel_source="$(awk -F= '/^Source=/{gsub(/\r/, "", $2); print $2; exit}' "$manifest")"
    public_symbols="$(awk -F= '/^PublicSymbols=/{gsub(/\r/, "", $2); print $2; exit}' "$manifest")"
    if [[ "$rel_source" == proofs/* ]]; then
        continue
    fi
    IFS=',' read -ra symbols <<< "$public_symbols"
    for symbol in "${symbols[@]}"; do
        symbol="${symbol// /}"
        [ -z "$symbol" ] && continue
        if ! grep -Eq "[[:space:]]${symbol}$" "$BUILD_DIR/OrynLibCHostConformance.nm"; then
            LogManifest "MISSING-LINKED-SYMBOL ${symbol} from ${manifest#${LIBC_ROOT}/FunctionManifests/}"
            coverage_failed=1
        else
            LogManifest "LINKED-SYMBOL ${symbol}"
        fi
    done
done

if [ "$coverage_failed" -ne 0 ]; then
    Fail "OrynLibC linked symbol inventory failed."
    Info "Coverage report: ${MANIFEST_REPORT}"
    exit 1
fi

Info "Running behavioral conformance checks."
if ! "$EXE"; then
    Fail "OrynLibC host behavioral conformance executable failed."
    Info "Report: ${REPORT}"
    exit 1
fi

LogReport "Manifest coverage: pass"
LogReport "Linked public symbol inventory: pass"
LogReport "Behavioral conformance executable: pass"
LogReport "Result: PASS"
Ok "OrynLibC host conformance passed: ${source_count} source unit(s), ${manifest_count} manifest(s)."
Ok "Report: ${REPORT}"
Ok "Coverage report: ${MANIFEST_REPORT}"
