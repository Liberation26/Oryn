#!/usr/bin/env bash
set -u

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SdkRoot="$(cd "$script_dir/../.." && pwd)"
WorkspaceRoot="$(cd "$SdkRoot/.." && pwd)"
current_script="$script_dir/UpdateOrynCurrent.sh"

export ORYN_WORKSPACE_ROOT="${ORYN_WORKSPACE_ROOT:-$WorkspaceRoot}"
export ORYN_SDK_ROOT="${ORYN_SDK_ROOT:-$SdkRoot}"
export ORYN_PROJECTS_ROOT="${ORYN_PROJECTS_ROOT:-$WorkspaceRoot/OrynProjects}"

ColorReset="\033[0m"
ColorFail="\033[31m"
if [ -n "${NO_COLOR:-}" ] || [ "${ORYN_NO_COLOR:-0}" = "1" ]; then
    Fail() { printf '[FAIL] %s\n' "$1" >&2; }
else
    Fail() { printf '%b[FAIL]%b %s\n' "$ColorFail" "$ColorReset" "$1" >&2; }
fi

if [ -f "$current_script" ]; then
    temp_script="$(mktemp)"
    cp "$current_script" "$temp_script"
    chmod +x "$temp_script"
    exec bash "$temp_script" "$@"
fi

Fail "UpdateOrynCurrent.sh was not found: $current_script"
exit 1
