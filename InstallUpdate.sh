#!/usr/bin/env bash
set -u

TargetRoot="${ORYN_SDK_ROOT:-/home/dave/OrynSDK}"
ScriptDir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ChangedRoot="$ScriptDir/ChangedFiles/OrynSDK"

Info() { printf '\033[36m[INFO]\033[0m %s\n' "$1"; }
Ok() { printf '\033[32m[ OK ]\033[0m %s\n' "$1"; }
Fail() { printf '\033[31m[FAIL]\033[0m %s\n' "$1" >&2; }

if [ ! -d "$ChangedRoot" ]; then
    Fail "ChangedFiles/OrynSDK was not found beside this script."
    exit 1
fi

mkdir -p "$TargetRoot"
Info "Applying ChangedFiles to: $TargetRoot"
cp -a "$ChangedRoot/." "$TargetRoot/"
chmod +x "$TargetRoot/update" "$TargetRoot/update.sh" "$TargetRoot/Scripts/UpdateOryn.sh" 2>/dev/null || true
Ok "Installed WSL updater."
Info "Next: cd $TargetRoot && ./update"
