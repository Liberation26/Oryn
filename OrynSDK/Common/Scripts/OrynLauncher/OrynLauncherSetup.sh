#!/usr/bin/env bash
set -u

ScriptPath="${ORYN_LAUNCHER_SCRIPT_PATH:-${BASH_SOURCE[0]}}"
while [ -L "$ScriptPath" ]; do
    ScriptDir="$(cd "$(dirname "$ScriptPath")" && pwd)"
    ScriptPath="$(readlink "$ScriptPath")"
    case "$ScriptPath" in
        /*) ;;
        *) ScriptPath="$ScriptDir/$ScriptPath" ;;
    esac
done

SdkRoot="$(cd "$(dirname "$ScriptPath")" && pwd)"
WorkspaceRoot="$(cd "$SdkRoot/.." && pwd)"
OrynBin="$SdkRoot/Common/Bin/oryn"
BuildScript="$SdkRoot/Common/Scripts/BuildOryn.sh"
DefaultProject="$WorkspaceRoot/OrynProjects/Kernel-5/Project.oryn"

ColorReset="\033[0m"
ColorInfo="\033[36m"
ColorOk="\033[32m"
ColorWarn="\033[33m"
ColorFail="\033[31m"
UseColor=1
[ -n "${NO_COLOR:-}" ] && UseColor=0
[ "${ORYN_NO_COLOR:-0}" = "1" ] && UseColor=0

PrintTag()
{
    Color="$1"
    Tag="$2"
    Message="$3"
    Stream="${4:-1}"

    if [ "$UseColor" -eq 1 ]; then
        if [ "$Stream" = "2" ]; then
            printf "%b%s%b %s\n" "$Color" "$Tag" "$ColorReset" "$Message" >&2
        else
            printf "%b%s%b %s\n" "$Color" "$Tag" "$ColorReset" "$Message"
        fi
    else
        if [ "$Stream" = "2" ]; then
            printf "%s %s\n" "$Tag" "$Message" >&2
        else
            printf "%s %s\n" "$Tag" "$Message"
        fi
    fi
}

Info() { PrintTag "$ColorInfo" "[INFO]" "$1"; }
Ok() { PrintTag "$ColorOk" "[ OK ]" "$1"; }
Warn() { PrintTag "$ColorWarn" "[WARN]" "$1"; }
Fail() { PrintTag "$ColorFail" "[FAIL]" "$1" 2; }

AskYesNoShell()
{
    Question="$1"
    DefaultYes="$2"

    while true; do
        if [ "$DefaultYes" -eq 1 ]; then
            printf '%s [Y/n]: ' "$Question"
        else
            printf '%s [y/N]: ' "$Question"
        fi
        IFS= read -r Answer || Answer=""
        Answer="$(printf '%s' "$Answer" | tr -d '\r' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
        case "$Answer" in
            '') return "$((DefaultYes ? 0 : 1))" ;;
            y|Y|yes|YES|Yes) return 0 ;;
            n|N|no|NO|No) return 1 ;;
            *) Warn "Please answer y or n." ;;
        esac
    done
}

ProjectRunDisplay()
{
    ProjectFile="$1"
    awk '
        BEGIN { section=""; display="" }
        /^[[:space:]]*\[/ {
            section=$0
            gsub(/^[[:space:]]*\[/, "", section)
            gsub(/\][[:space:]]*$/, "", section)
            next
        }
        section == "Run" && /^[[:space:]]*Display[[:space:]]*=/ {
            sub(/^[[:space:]]*Display[[:space:]]*=[[:space:]]*/, "")
            gsub(/\r/, "")
            display=$0
        }
        END { print display }
    ' "$ProjectFile"
}

WriteProjectRunDisplay()
{
    ProjectFile="$1"
    DisplayValue="$2"
    TempFile="$(mktemp)"

    awk -v display="$DisplayValue" '
        BEGIN { section=""; saw_run=0; wrote_display=0 }
        /^[[:space:]]*\[/ {
            if (section == "Run" && wrote_display == 0) {
                print "Display=" display
                wrote_display=1
            }
            section=$0
            gsub(/^[[:space:]]*\[/, "", section)
            gsub(/\][[:space:]]*$/, "", section)
            gsub(/\r/, "", section)
            if (section == "Run") {
                saw_run=1
            }
            print
            next
        }
        section == "Run" && /^[[:space:]]*Display[[:space:]]*=/ {
            if (wrote_display == 0) {
                print "Display=" display
                wrote_display=1
            }
            next
        }
        { print }
        END {
            if (saw_run == 0) {
                print ""
                print "[Run]"
                print "VM=QEMU"
                print "Memory=512M"
                print "Serial=stdio"
                print "Display=" display
            } else if (section == "Run" && wrote_display == 0) {
                print "Display=" display
            }
        }
    ' "$ProjectFile" > "$TempFile" || {
        rm -f "$TempFile"
        return 1
    }

    cat "$TempFile" > "$ProjectFile"
    rm -f "$TempFile"
    return 0
}



ProjectRunValue()
{
    ProjectFile="$1"
    Key="$2"
    DefaultValue="${3:-}"
    awk -v wanted="$Key" -v default_value="$DefaultValue" '
        BEGIN { section=""; value=default_value }
        /^[[:space:]]*\[/ {
            section=$0
            gsub(/^[[:space:]]*\[/, "", section)
            gsub(/\][[:space:]]*$/, "", section)
            gsub(/\r/, "", section)
            next
        }
        section == "Run" {
            line=$0
            gsub(/\r/, "", line)
            split(line, parts, "=")
            key=parts[1]
            gsub(/^[[:space:]]*/, "", key)
            gsub(/[[:space:]]*$/, "", key)
            if (key == wanted) {
                sub(/^[^=]*=[[:space:]]*/, "", line)
                value=line
            }
        }
        END { print value }
    ' "$ProjectFile"
}

WriteProjectRunSetting()
{
    ProjectFile="$1"
    Key="$2"
    Value="$3"
    TempFile="$(mktemp)"

    awk -v key="$Key" -v value="$Value" '
        BEGIN { section=""; saw_run=0; wrote_key=0 }
        /^[[:space:]]*\[/ {
            if (section == "Run" && wrote_key == 0) {
                print key "=" value
                wrote_key=1
            }
            section=$0
            gsub(/^[[:space:]]*\[/, "", section)
            gsub(/\][[:space:]]*$/, "", section)
            gsub(/\r/, "", section)
            if (section == "Run") {
                saw_run=1
            }
            print
            next
        }
        section == "Run" {
            line=$0
            gsub(/\r/, "", line)
            probe=line
            split(probe, parts, "=")
            current=parts[1]
            gsub(/^[[:space:]]*/, "", current)
            gsub(/[[:space:]]*$/, "", current)
            if (current == key) {
                if (wrote_key == 0) {
                    print key "=" value
                    wrote_key=1
                }
                next
            }
        }
        { print }
        END {
            if (saw_run == 0) {
                print ""
                print "[Run]"
                print key "=" value
            } else if (section == "Run" && wrote_key == 0) {
                print key "=" value
            }
        }
    ' "$ProjectFile" > "$TempFile" || {
        rm -f "$TempFile"
        return 1
    }

    cat "$TempFile" > "$ProjectFile"
    rm -f "$TempFile"
    return 0
}

IsValidVmValueShell()
{
    Candidate="$1"
    case "$Candidate" in
        ''|*[!ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_+.,:=/-]*) return 1 ;;
        *) return 0 ;;
    esac
}

AskValueShell()
{
    Question="$1"
    DefaultValue="$2"
    Validator="$3"

    while true; do
        printf '%s [%s]: ' "$Question" "${DefaultValue:-<empty>}" >&2
        IFS= read -r Answer || Answer=""
        Answer="$(printf '%s' "$Answer" | tr -d '\r' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
        [ -n "$Answer" ] || Answer="$DefaultValue"
        [ "$Answer" = "<empty>" ] && Answer=""

        case "$Validator" in
            vm)
                if IsValidVmValueShell "$Answer"; then
                    printf '%s\n' "$Answer"
                    return 0
                fi
                printf '[WARN] Use letters, numbers, underscore, dash, plus, comma, dot, colon, slash or equals only.\n' >&2
                ;;
            number)
                case "$Answer" in
                    ''|*[!0123456789]*) printf '[WARN] Use a whole number.\n' >&2 ;;
                    *) printf '%s\n' "$Answer"; return 0 ;;
                esac
                ;;
            *)
                printf '%s\n' "$Answer"
                return 0
                ;;
        esac
    done
}


NormalizeQemuCpuForWslShell()
{
    CpuValue="$1"
    case "$CpuValue" in
        host|Host|HOST|native|Native|NATIVE)
            PrintTag "$ColorWarn" "[WARN]" "CPU=$CpuValue requires KVM/HVF/WHX acceleration and is not valid for the current Windows QEMU from WSL runner." 2
            PrintTag "$ColorWarn" "[WARN]" "Saving CPU=max instead. Use qemu64 for the most conservative profile." 2
            printf 'max\n'
            ;;
        '')
            printf 'qemu64\n'
            ;;
        *)
            printf '%s\n' "$CpuValue"
            ;;
    esac
}

