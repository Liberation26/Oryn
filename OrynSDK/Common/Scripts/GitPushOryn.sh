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

ScriptDir="$(cd "$(dirname "$ScriptPath")" && pwd)"
export ORYN_GITPUSH_SCRIPT_PATH="$ScriptPath"

. "$ScriptDir/GitPushOryn/GitPushOrynSetup.sh"
. "$ScriptDir/GitPushOryn/GitPushOrynMain.sh"
