#!/usr/bin/env bash
set -u

ScriptPath="${BASH_SOURCE[0]}"
while [ -L "$ScriptPath" ]; do
    ScriptDir="$(cd "$(dirname "$ScriptPath")" && pwd)"
    ScriptPath="$(readlink "$ScriptPath")"
    case "$ScriptPath" in
        /*) ;;
        *) ScriptPath="$ScriptDir/$ScriptPath" ;;
    esac
done

SdkRoot="$(cd "$(dirname "$ScriptPath")" && pwd)"
export ORYN_LAUNCHER_SCRIPT_PATH="$ScriptPath"

. "$SdkRoot/Common/Scripts/OrynLauncher/OrynLauncherSetup.sh"
. "$SdkRoot/Common/Scripts/OrynLauncher/OrynLauncherActions.sh"
. "$SdkRoot/Common/Scripts/OrynLauncher/OrynLauncherMain.sh"
