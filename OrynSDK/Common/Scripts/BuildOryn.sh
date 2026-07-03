#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

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
    local stream="${4:-1}"

    if [ "$UseColor" -eq 1 ]; then
        if [ "$stream" = "2" ]; then
            printf "%b%s%b %s\n" "$color" "$tag" "$ColorReset" "$message" >&2
        else
            printf "%b%s%b %s\n" "$color" "$tag" "$ColorReset" "$message"
        fi
    else
        if [ "$stream" = "2" ]; then
            printf "%s %s\n" "$tag" "$message" >&2
        else
            printf "%s %s\n" "$tag" "$message"
        fi
    fi
}

Info() { PrintTag "$ColorInfo" "[INFO]" "$1"; }
Ok() { PrintTag "$ColorOk" "[ OK ]" "$1"; }
Fail() { PrintTag "$ColorFail" "[FAIL]" "$1" 2; }

mkdir -p "${SDK_ROOT}/Common/Bin"

if ! command -v clang >/dev/null 2>&1; then
    Fail "clang was not found in WSL."
    Info "Install with: sudo apt install -y clang lld llvm"
    exit 1
fi


RequireBuildSource()
{
    local relative="$1"
    if [ ! -f "${SDK_ROOT}/${relative}" ]; then
        Fail "Bootstrap source missing: ${relative}"
        Info "Run ./update or ./update.sh with the latest OrynWsl package, or apply a full update."
        exit 1
    fi
}

RequireBuildSource "Targets/UEFI/X64/OrynBuild/BuildArchivePaths.c"
RequireBuildSource "Targets/UEFI/X64/OrynBuild/BuildArchiveCompile.c"
RequireBuildSource "Targets/UEFI/X64/OrynBuild/BuildArchiveManifest.c"
RequireBuildSource "Targets/UEFI/X64/OrynBuild/BuildModuleAdd.c"
RequireBuildSource "Targets/UEFI/X64/OrynBuild/BuildPlanModules.c"
RequireBuildSource "Targets/UEFI/X64/OrynBuild/BuildKernelModuleManifests.c"
RequireBuildSource "Targets/UEFI/X64/OrynBuild/BuildKernelModuleManifestRead.c"
RequireBuildSource "Targets/UEFI/X64/OrynBuild/BuildKernelModuleManifestValidate.c"
RequireBuildSource "Targets/UEFI/X64/OrynBuild/ManifestValidationCommand.c"
RequireBuildSource "Targets/UEFI/X64/OrynBuild/BuildLibCFunctionManifests.c"
RequireBuildSource "Common/OrynLibC/FunctionManifests/string/strlen.libcunit"
RequireBuildSource "Common/BuildModuleManifests/00-OrynLibC.buildmodule"
RequireBuildSource "Targets/UEFI/X64/BuildModuleManifests/12-TargetRuntime.buildmodule"

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
Ok "Built ${SDK_ROOT}/Common/Bin/oryn"
