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
CaseDir="$(basename "$ScriptDir")"
if [ "$CaseDir" = "Scripts" ]; then
    WorkspaceRoot="${ORYN_WORKSPACE_ROOT:-$(cd "$ScriptDir/../../.." && pwd)}"
elif [ -d "$ScriptDir/OrynSDK" ]; then
    WorkspaceRoot="$ScriptDir"
elif [ "$CaseDir" = "OrynSDK" ]; then
    WorkspaceRoot="$(cd "$ScriptDir/.." && pwd)"
else
    WorkspaceRoot="$(cd "$ScriptDir" && pwd)"
fi

GitIgnorePath="$WorkspaceRoot/.gitignore"
RemoteAddressPath="$WorkspaceRoot/GitHubRepo.address"
DefaultBranch="main"
RemoteName="origin"
CommitMessage="Oryn source update"
ExplicitRemote=""
ForceWithLease=0
DryRun=0
SetupGcmOnly=0
AutoSetupGcm=1

Info() { printf '\033[36m[INFO]\033[0m %s\n' "$1"; }
Ok() { printf '\033[32m[ OK ]\033[0m %s\n' "$1"; }
Warn() { printf '\033[33m[WARN]\033[0m %s\n' "$1"; }
Fail() { printf '\033[31m[FAIL]\033[0m %s\n' "$1" >&2; }

Usage()
{
    cat <<EOF
Oryn GitPush

Usage:
  ./GitPush.sh [options] [github-url]
  ./oryn gitpush [options] [github-url]

Options:
  --message, -m <text>       Commit message. Default: Oryn source update
  --branch, -b <name>        Branch to create/use for a new repo. Default: main
  --remote-name <name>       Git remote name. Default: origin
  --force-with-lease         Push with --force-with-lease instead of a normal push
  --dry-run                  Prepare repo and commit, but do not push
  --setup-gcm-only           Configure Git Credential Manager for WSL, then exit
  --no-setup-gcm             Do not auto-configure Git Credential Manager
  --help, -h                 Show this help

Remote selection order:
  1. github-url argument
  2. first non-comment line in GitHubRepo.address
  3. existing origin remote URL

GitHubRepo.address may contain comments, for example:
  https://github.com/YourName/YourRepo.git # Replace this repo address with your own
EOF
}

Trim()
{
    printf '%s' "$1" | tr -d '\r' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//'
}

LooksLikeGitUrl()
{
    case "$1" in
        https://*|http://*|ssh://*|git@*:*) return 0 ;;
        *) return 1 ;;
    esac
}

ReadAddressFile()
{
    AddressFile="$1"
    [ -f "$AddressFile" ] || return 1

    while IFS= read -r RawLine || [ -n "$RawLine" ]; do
        Line="$(Trim "$RawLine")"
        [ -n "$Line" ] || continue
        case "$Line" in
            \#*) continue ;;
        esac
        Line="$(printf '%s' "$Line" | sed 's/[[:space:]]#.*$//')"
        Line="$(Trim "$Line")"
        [ -n "$Line" ] || continue
        printf '%s\n' "$Line"
        return 0
    done < "$AddressFile"

    return 1
}

RemoteUrlFromGit()
{
    git -C "$WorkspaceRoot" remote get-url "$RemoteName" 2>/dev/null || true
}

ParseArgs()
{
    while [ "$#" -gt 0 ]; do
        case "$1" in
            --help|-h)
                Usage
                exit 0
                ;;
            --message|-m)
                shift || true
                if [ "$#" -eq 0 ]; then
                    Fail "Missing commit message after --message."
                    exit 1
                fi
                CommitMessage="$1"
                ;;
            --branch|-b)
                shift || true
                if [ "$#" -eq 0 ]; then
                    Fail "Missing branch name after --branch."
                    exit 1
                fi
                DefaultBranch="$1"
                ;;
            --remote-name)
                shift || true
                if [ "$#" -eq 0 ]; then
                    Fail "Missing remote name after --remote-name."
                    exit 1
                fi
                RemoteName="$1"
                ;;
            --force-with-lease)
                ForceWithLease=1
                ;;
            --dry-run)
                DryRun=1
                ;;
            --setup-gcm-only|--git-login|--login)
                SetupGcmOnly=1
                ;;
            --no-setup-gcm)
                AutoSetupGcm=0
                ;;
            --*)
                Fail "Unknown GitPush option: $1"
                Usage
                exit 1
                ;;
            *)
                if [ -z "$ExplicitRemote" ] && LooksLikeGitUrl "$1"; then
                    ExplicitRemote="$1"
                elif [ "$CommitMessage" = "Oryn source update" ]; then
                    CommitMessage="$1"
                else
                    Fail "Unexpected argument: $1"
                    Usage
                    exit 1
                fi
                ;;
        esac
        shift || true
    done
}

ResolveRemoteUrl()
{
    if [ -n "$ExplicitRemote" ]; then
        printf '%s\n' "$ExplicitRemote"
        return 0
    fi

    FileRemote="$(ReadAddressFile "$RemoteAddressPath" 2>/dev/null || true)"
    if [ -n "$FileRemote" ]; then
        printf '%s\n' "$FileRemote"
        return 0
    fi

    ExistingRemote="$(RemoteUrlFromGit)"
    if [ -n "$ExistingRemote" ]; then
        printf '%s\n' "$ExistingRemote"
        return 0
    fi

    return 1
}

WriteGitIgnore()
{
    TempFile="$(mktemp)"

    if [ -f "$GitIgnorePath" ]; then
        awk '
            /^# BEGIN ORYN GENERATED OUTPUT IGNORE$/ { skip=1; next }
            /^# END ORYN GENERATED OUTPUT IGNORE$/ { skip=0; next }
            skip == 0 { print }
        ' "$GitIgnorePath" > "$TempFile"
    else
        : > "$TempFile"
    fi

    cat >> "$TempFile" <<'EOF'
# BEGIN ORYN GENERATED OUTPUT IGNORE

# Oryn build and runtime output
Build/
Output/
Debug/
Release/
*.log
*.tmp
*.cache
*.d
*.map
Debug.log
BootReport.txt

# Kernel/project generated output
OrynProjects/*/Build/
OrynProjects/*/Output/
OrynProjects/*/Build*/
OrynProjects/*/Output*/
OrynProjects/*/.cache/

# SDK generated binaries and object output
OrynSDK/Common/Bin/*
!OrynSDK/Common/Bin/.gitkeep
OrynSDK/**/Build/
OrynSDK/**/Output/
OrynSDK/**/*.o
OrynSDK/**/*.obj
OrynSDK/**/*.a
OrynSDK/**/*.lib
OrynSDK/**/*.so
OrynSDK/**/*.dll
OrynSDK/**/*.dylib
OrynSDK/**/*.exe
OrynSDK/**/*.pdb

# Boot images and linked artifacts
*.o
*.obj
*.a
*.lib
*.so
*.dll
*.dylib
*.exe
*.pdb
*.efi
*.elf
*.bin
*.img
*.iso
*.vhd
*.vhdx
*.qcow2

# Editor and host noise
.vs/
.vscode/.ropeproject/
.idea/
.DS_Store
Thumbs.db
__pycache__/
*.pyc

# END ORYN GENERATED OUTPUT IGNORE
EOF

    cat "$TempFile" > "$GitIgnorePath"
    rm -f "$TempFile"
    Ok "Updated .gitignore with Oryn output exclusions."
}

EnsureRemoteAddressFile()
{
    if [ -f "$RemoteAddressPath" ]; then
        return 0
    fi

    cat > "$RemoteAddressPath" <<'EOF'
# Replace this repo address with your own.
# GitPush uses the first non-empty line that does not start with #.
https://github.com/Liberation26/Oryn.git # Replace this repo address with your own
EOF
    Ok "Created GitHubRepo.address. Edit it if you want a different GitHub repo."
}

EnsureGitIdentity()
{
    if ! git -C "$WorkspaceRoot" config user.name >/dev/null 2>&1; then
        git -C "$WorkspaceRoot" config user.name "Oryn GitPush" || return 1
        Ok "Set local git user.name to Oryn GitPush."
    fi

    if ! git -C "$WorkspaceRoot" config user.email >/dev/null 2>&1; then
        git -C "$WorkspaceRoot" config user.email "oryn-gitpush@local" || return 1
        Ok "Set local git user.email to oryn-gitpush@local."
    fi

    return 0
}

EnsureRepo()
{
    if git -C "$WorkspaceRoot" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        Ok "Existing git repository detected."
        return 0
    fi

    if git init -b "$DefaultBranch" "$WorkspaceRoot" >/dev/null 2>&1; then
        Ok "Initialised local git repository on branch $DefaultBranch."
        return 0
    fi

    git init "$WorkspaceRoot" >/dev/null || return 1
    Ok "Initialised local git repository."

    git -C "$WorkspaceRoot" checkout -B "$DefaultBranch" >/dev/null 2>&1 || return 1
    Ok "Created branch $DefaultBranch."
    return 0
}

EnsureBranch()
{
    if ! git -C "$WorkspaceRoot" rev-parse --verify HEAD >/dev/null 2>&1; then
        git -C "$WorkspaceRoot" checkout -B "$DefaultBranch" >/dev/null 2>&1 || return 1
        Ok "Using branch $DefaultBranch."
        return 0
    fi

    CurrentBranch="$(git -C "$WorkspaceRoot" branch --show-current 2>/dev/null || true)"
    if [ -z "$CurrentBranch" ]; then
        git -C "$WorkspaceRoot" checkout -B "$DefaultBranch" >/dev/null 2>&1 || return 1
        Ok "Detached HEAD moved to branch $DefaultBranch."
    else
        Ok "Using existing branch $CurrentBranch."
    fi

    return 0
}

EnsureRemote()
{
    RemoteUrl="$1"

    Existing="$(RemoteUrlFromGit)"
    if [ -n "$Existing" ]; then
        if [ "$Existing" != "$RemoteUrl" ]; then
            git -C "$WorkspaceRoot" remote set-url "$RemoteName" "$RemoteUrl" || return 1
            Ok "Updated $RemoteName remote URL."
        else
            Ok "Remote $RemoteName already points to $RemoteUrl."
        fi
    else
        git -C "$WorkspaceRoot" remote add "$RemoteName" "$RemoteUrl" || return 1
        Ok "Added remote $RemoteName -> $RemoteUrl."
    fi

    return 0
}

UntrackGeneratedOutput()
{
    git -C "$WorkspaceRoot" rm -r --cached --ignore-unmatch \
        Build Output Debug Release .vs .idea \
        OrynSDK/Common/Bin/oryn \
        OrynSDK/Common/Bin/oryn.exe \
        OrynProjects/*/Build OrynProjects/*/Output \
        >/dev/null 2>&1 || true

    while IFS= read -r GeneratedPath; do
        [ -n "$GeneratedPath" ] || continue
        RelPath="${GeneratedPath#$WorkspaceRoot/}"
        git -C "$WorkspaceRoot" rm -r --cached --ignore-unmatch "$RelPath" >/dev/null 2>&1 || true
    done <<EOF
$(find "$WorkspaceRoot" -type d \( -name Build -o -name Output -o -name '.cache' \) -print 2>/dev/null)
EOF

    Ok "Removed generated output from the git index where present."
}

CommitChanges()
{
    git -C "$WorkspaceRoot" add -A || return 1

    if git -C "$WorkspaceRoot" diff --cached --quiet --exit-code; then
        if git -C "$WorkspaceRoot" rev-parse --verify HEAD >/dev/null 2>&1; then
            Ok "No source changes to commit."
            return 0
        fi

        git -C "$WorkspaceRoot" commit --allow-empty -m "Initial Oryn source tree" >/dev/null || return 1
        Ok "Created empty initial commit."
        return 0
    fi

    git -C "$WorkspaceRoot" commit -m "$CommitMessage" >/dev/null || return 1
    Ok "Committed source tree: $CommitMessage"
    return 0
}

PushChanges()
{
    Branch="$(git -C "$WorkspaceRoot" branch --show-current 2>/dev/null || true)"
    [ -n "$Branch" ] || Branch="$DefaultBranch"

    if [ "$DryRun" -eq 1 ]; then
        Warn "Dry run requested. Repository prepared and committed, but not pushed."
        return 0
    fi

    if [ "$ForceWithLease" -eq 1 ]; then
        git -C "$WorkspaceRoot" push --force-with-lease -u "$RemoteName" "$Branch" || return 1
    else
        git -C "$WorkspaceRoot" push -u "$RemoteName" "$Branch" || return 1
    fi

    Ok "Pushed $Branch to $RemoteName."
    return 0
}

EscapeCredentialHelperPath()
{
    printf '%s' "$1" | sed 's/\\/\\\\/g; s/ /\\ /g; s/(/\\(/g; s/)/\\)/g'
}

WindowsUserName()
{
    if command -v cmd.exe >/dev/null 2>&1; then
        cmd.exe /C echo %USERNAME% 2>/dev/null | tr -d '\r' | tail -n 1
        return 0
    fi

    if [ -d /mnt/c/Users ]; then
        find /mnt/c/Users -mindepth 1 -maxdepth 1 -type d -printf '%f\n' 2>/dev/null | grep -viE '^(Public|Default|Default User|All Users)$' | head -n 1
        return 0
    fi

    return 1
}

FindWindowsGitCredentialManager()
{
    WinUser="$(WindowsUserName 2>/dev/null || true)"

    for Candidate in \
        "/mnt/c/Program Files/Git/mingw64/bin/git-credential-manager.exe" \
        "/mnt/c/Program Files/Git/mingw64/libexec/git-core/git-credential-manager.exe" \
        "/mnt/c/Program Files/Git/usr/bin/git-credential-manager.exe" \
        "/mnt/c/Program Files (x86)/Git Credential Manager/git-credential-manager.exe" \
        "/mnt/c/Users/$WinUser/AppData/Local/Programs/Git Credential Manager/git-credential-manager.exe"; do
        [ -n "$Candidate" ] || continue
        [ -f "$Candidate" ] || continue
        printf '%s\n' "$Candidate"
        return 0
    done

    return 1
}

ConfigureGitCredentialManager()
{
    if [ "$AutoSetupGcm" -ne 1 ]; then
        Warn "Git Credential Manager auto-setup is disabled for this run."
        return 0
    fi

    ExistingHelper="$(git config --global --get credential.helper 2>/dev/null || true)"
    case "$ExistingHelper" in
        *git-credential-manager*|*manager-core*)
            Ok "Git Credential Manager is already configured for WSL git."
            return 0
            ;;
    esac

    WindowsGcm="$(FindWindowsGitCredentialManager 2>/dev/null || true)"
    if [ -n "$WindowsGcm" ]; then
        EscapedHelper="$(EscapeCredentialHelperPath "$WindowsGcm")"
        git config --global credential.helper "$EscapedHelper" || return 1
        Ok "Configured WSL git to use Windows Git Credential Manager."
        Info "GCM path: $WindowsGcm"
        return 0
    fi

    if command -v git-credential-manager >/dev/null 2>&1; then
        if git-credential-manager configure >/dev/null 2>&1; then
            Ok "Configured Linux Git Credential Manager inside WSL."
            return 0
        fi
    fi

    Warn "Git Credential Manager was not found. Git may ask for credentials or the push may fail."
    Warn "Install Git for Windows with Git Credential Manager, then run ./Oryn.sh gitlogin."
    return 1
}

ParseArgs "$@"

if [ "$SetupGcmOnly" -eq 1 ]; then
    Info "Configuring GitHub login support for WSL git."
    if ! command -v git >/dev/null 2>&1; then
        Fail "git was not found in PATH. Install git inside WSL first."
        exit 1
    fi
    ConfigureGitCredentialManager || { Fail "Could not configure Git Credential Manager."; exit 1; }
    Ok "GitHub login support configured. The next HTTPS push may open a browser/device login prompt."
    exit 0
fi

Info "Oryn GitPush starting."
Info "Workspace: $WorkspaceRoot"

if ! command -v git >/dev/null 2>&1; then
    Fail "git was not found in PATH. Install git first, then run GitPush again."
    exit 1
fi

ConfigureGitCredentialManager || true

EnsureRemoteAddressFile
RemoteUrl="$(ResolveRemoteUrl 2>/dev/null || true)"
if [ -z "$RemoteUrl" ]; then
    Fail "No GitHub remote URL was supplied and GitHubRepo.address does not contain one."
    Warn "Edit $RemoteAddressPath or run: ./GitPush.sh https://github.com/YourName/YourRepo.git"
    exit 1
fi

WriteGitIgnore
EnsureRepo || { Fail "Could not initialise git repository."; exit 1; }
EnsureBranch || { Fail "Could not select git branch."; exit 1; }
EnsureGitIdentity || { Fail "Could not configure local git identity."; exit 1; }
EnsureRemote "$RemoteUrl" || { Fail "Could not configure git remote."; exit 1; }
UntrackGeneratedOutput
CommitChanges || { Fail "Could not commit source tree."; exit 1; }

if ! PushChanges; then
    Fail "Git push failed."
    Warn "If authentication failed, run ./Oryn.sh gitlogin and then retry ./Oryn.sh gitpush."
    Warn "If the GitHub repo already has unrelated commits, pull/rebase first or rerun with --force-with-lease only if you intentionally want this tree to replace the remote branch."
    exit 1
fi

Ok "GitPush complete."
