#!/usr/bin/env bash
set -u

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SdkRoot="$(cd "$script_dir/.." && pwd)"
WorkspaceRoot="$(cd "$SdkRoot/.." && pwd)"
current_script="$script_dir/UpdateOrynCurrent.sh"

export ORYN_WORKSPACE_ROOT="${ORYN_WORKSPACE_ROOT:-$WorkspaceRoot}"
export ORYN_SDK_ROOT="${ORYN_SDK_ROOT:-$SdkRoot}"
export ORYN_PROJECTS_ROOT="${ORYN_PROJECTS_ROOT:-$WorkspaceRoot/OrynProjects}"

if [ -f "$current_script" ]; then
    temp_script="$(mktemp)"
    cp "$current_script" "$temp_script"
    chmod +x "$temp_script"
    exec bash "$temp_script" "$@"
fi

printf '[FAIL] UpdateOrynCurrent.sh was not found: %s\n' "$current_script" >&2
exit 1
