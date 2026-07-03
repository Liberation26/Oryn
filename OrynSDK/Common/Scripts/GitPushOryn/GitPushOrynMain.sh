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

FindWindowsGitRoot()
{
    for Candidate in \
        "/mnt/c/Program Files/Git" \
        "/mnt/c/Program Files (x86)/Git"; do
        [ -x "$Candidate/cmd/git.exe" ] || [ -x "$Candidate/bin/git.exe" ] || continue
        printf '%s\n' "$Candidate"
        return 0
    done

    return 1
}

FindWindowsGitCredentialManager()
{
    WinUser="$(WindowsUserName 2>/dev/null || true)"

    for Candidate in \
        "/mnt/c/Program Files/Git/mingw64/bin/git-credential-manager.exe" \
        "/mnt/c/Program Files/Git/mingw64/libexec/git-core/git-credential-manager.exe" \
        "/mnt/c/Program Files/Git/usr/bin/git-credential-manager.exe" \
        "/mnt/c/Program Files (x86)/Git/mingw64/bin/git-credential-manager.exe" \
        "/mnt/c/Program Files (x86)/Git/mingw64/libexec/git-core/git-credential-manager.exe" \
        "/mnt/c/Program Files (x86)/Git Credential Manager/git-credential-manager.exe" \
        "/mnt/c/Users/$WinUser/AppData/Local/Programs/Git Credential Manager/git-credential-manager.exe"; do
        [ -n "$Candidate" ] || continue
        [ -f "$Candidate" ] || continue
        printf '%s\n' "$Candidate"
        return 0
    done

    return 1
}

BuildWindowsGitPathPrefix()
{
    GitRoot="$(FindWindowsGitRoot 2>/dev/null || true)"
    [ -n "$GitRoot" ] || return 1

    First=1
    for Candidate in \
        "$GitRoot/cmd" \
        "$GitRoot/bin" \
        "$GitRoot/mingw64/bin" \
        "$GitRoot/usr/bin" \
        "$GitRoot/mingw64/libexec/git-core"; do
        [ -d "$Candidate" ] || continue
        if [ "$First" -eq 1 ]; then
            printf '%s' "$Candidate"
            First=0
        else
            printf ':%s' "$Candidate"
        fi
    done

    [ "$First" -eq 0 ]
}

BuildWindowsGcmBridgeScript()
{
    BridgeDir="$HOME/.oryn"
    BridgePath="$BridgeDir/git-credential-manager-wsl-bridge.sh"
    mkdir -p "$BridgeDir" || return 1

    cat > "$BridgePath" <<'EOF'
#!/usr/bin/env bash
set -u

# Oryn WSL Git Credential Manager bridge.
# Git credential helpers receive credential fields on stdin.  Some Windows
# discovery commands can accidentally consume that stdin when launched from
# WSL, so capture it first and replay it only to Git Credential Manager.
InputFile="$(mktemp "${TMPDIR:-/tmp}/oryn-gcm-input.XXXXXX")" || exit 1
cat > "$InputFile"
cleanup()
{
    rm -f "$InputFile"
}
trap cleanup EXIT HUP INT TERM

FindWindowsGitRoot()
{
    for Candidate in \
        "/mnt/c/Program Files/Git" \
        "/mnt/c/Program Files (x86)/Git"; do
        [ -x "$Candidate/cmd/git.exe" ] || [ -x "$Candidate/bin/git.exe" ] || continue
        printf '%s\n' "$Candidate"
        return 0
    done

    return 1
}

WindowsUserName()
{
    if command -v cmd.exe >/dev/null 2>&1; then
        cmd.exe /C echo %USERNAME% </dev/null 2>/dev/null | tr -d '\r' | tail -n 1
        return 0
    fi

    return 1
}

FindWindowsGitCredentialManager()
{
    for Candidate in \
        "/mnt/c/Program Files/Git/mingw64/bin/git-credential-manager.exe" \
        "/mnt/c/Program Files/Git/mingw64/libexec/git-core/git-credential-manager.exe" \
        "/mnt/c/Program Files/Git/usr/bin/git-credential-manager.exe" \
        "/mnt/c/Program Files (x86)/Git/mingw64/bin/git-credential-manager.exe" \
        "/mnt/c/Program Files (x86)/Git/mingw64/libexec/git-core/git-credential-manager.exe" \
        "/mnt/c/Program Files (x86)/Git Credential Manager/git-credential-manager.exe"; do
        [ -f "$Candidate" ] || continue
        printf '%s\n' "$Candidate"
        return 0
    done

    WinUser="$(WindowsUserName 2>/dev/null || true)"
    if [ -n "$WinUser" ]; then
        Candidate="/mnt/c/Users/$WinUser/AppData/Local/Programs/Git Credential Manager/git-credential-manager.exe"
        if [ -f "$Candidate" ]; then
            printf '%s\n' "$Candidate"
            return 0
        fi
    fi

    return 1
}

BuildWindowsGitPathPrefix()
{
    GitRoot="$(FindWindowsGitRoot 2>/dev/null || true)"
    [ -n "$GitRoot" ] || return 1

    First=1
    for Candidate in \
        "$GitRoot/cmd" \
        "$GitRoot/bin" \
        "$GitRoot/mingw64/bin" \
        "$GitRoot/usr/bin" \
        "$GitRoot/mingw64/libexec/git-core"; do
        [ -d "$Candidate" ] || continue
        if [ "$First" -eq 1 ]; then
            printf '%s' "$Candidate"
            First=0
        else
            printf ':%s' "$Candidate"
        fi
    done

    [ "$First" -eq 0 ]
}

PrepareWindowsGitPathForGcm()
{
    GitPathPrefix="$(BuildWindowsGitPathPrefix 2>/dev/null || true)"
    [ -n "$GitPathPrefix" ] || return 0

    export PATH="$GitPathPrefix:$PATH"

    # Ask WSL interop to translate the Linux path-list into a Windows path-list
    # for the Windows GCM process.  This lets GCM locate git.exe.
    case ":${WSLENV:-}:" in
        *":PATH/l:"*) ;;
        *) export WSLENV="${WSLENV:+$WSLENV:}PATH/l" ;;
    esac
}

RunDirectoryForWindowsExe()
{
    GitRoot="$(FindWindowsGitRoot 2>/dev/null || true)"
    if [ -n "$GitRoot" ] && [ -d "$GitRoot" ]; then
        printf '%s\n' "$GitRoot"
        return 0
    fi

    if [ -d /mnt/c/Windows/System32 ]; then
        printf '%s\n' /mnt/c/Windows/System32
        return 0
    fi

    pwd
}

GcmPath="$(FindWindowsGitCredentialManager 2>/dev/null || true)"
PrepareWindowsGitPathForGcm

RunDir="$(RunDirectoryForWindowsExe 2>/dev/null || pwd)"
cd "$RunDir" 2>/dev/null || true

if [ -n "$GcmPath" ]; then
    "$GcmPath" "$@" < "$InputFile"
    exit "$?"
fi

if command -v git-credential-manager >/dev/null 2>&1; then
    git-credential-manager "$@" < "$InputFile"
    exit "$?"
fi

printf 'Oryn GCM bridge could not find Git Credential Manager.\n' >&2
exit 1
EOF

    chmod +x "$BridgePath" || return 1
    printf '%s\n' "$BridgePath"
    return 0
}

BuildWindowsGcmHelperCommand()
{
    BridgePath="$(BuildWindowsGcmBridgeScript 2>/dev/null || true)"
    if [ -n "$BridgePath" ]; then
        printf '%s\n' "$BridgePath"
        return 0
    fi

    WindowsGcm="$1"
    GitPathPrefix="$(BuildWindowsGitPathPrefix 2>/dev/null || true)"

    if [ -n "$GitPathPrefix" ]; then
        printf '!f() { export PATH="%s:$PATH"; case ":${WSLENV:-}:" in *":PATH/l:"*) ;; *) export WSLENV="${WSLENV:+$WSLENV:}PATH/l" ;; esac; "%s" "$@"; }; f\n' "$GitPathPrefix" "$WindowsGcm"
    else
        printf '!f() { "%s" "$@"; }; f\n' "$WindowsGcm"
    fi
}

PrepareWindowsGitPathForGcm()
{
    GitPathPrefix="$(BuildWindowsGitPathPrefix 2>/dev/null || true)"
    [ -n "$GitPathPrefix" ] || return 0

    export PATH="$GitPathPrefix:$PATH"

    case ":${WSLENV:-}:" in
        *":PATH/l:"*) ;;
        *) export WSLENV="${WSLENV:+$WSLENV:}PATH/l" ;;
    esac
}

ConfigureGitCredentialManager()
{
    if [ "$AutoSetupGcm" -ne 1 ]; then
        Warn "Git Credential Manager auto-setup is disabled for this run."
        return 0
    fi

    PrepareWindowsGitPathForGcm

    ExistingHelper="$(git config --global --get credential.helper 2>/dev/null || true)"
    case "$ExistingHelper" in
        */.oryn/git-credential-manager-wsl-bridge.sh)
            BridgePath="$(BuildWindowsGcmBridgeScript 2>/dev/null || true)"
            if [ -n "$BridgePath" ]; then
                Ok "Git Credential Manager is configured with the refreshed Oryn WSL bridge."
                return 0
            fi
            Warn "Existing Oryn Git Credential Manager bridge could not be refreshed. Recreating it."
            ;;
        '!'*git-credential-manager*'WSLENV'*'PATH/l'*)
            Ok "Git Credential Manager is already configured with WSL PATH translation."
            return 0
            ;;
        '!'*git-credential-manager*'export PATH='*)
            Warn "Existing Git Credential Manager helper lacks WSL PATH translation. Reconfiguring it."
            ;;
        *git-credential-manager.exe*)
            Warn "Existing Git Credential Manager helper may not expose Windows git.exe to GCM. Reconfiguring it."
            ;;
        *manager-core*)
            Ok "Git Credential Manager is already configured for WSL git."
            return 0
            ;;
    esac

    WindowsGcm="$(FindWindowsGitCredentialManager 2>/dev/null || true)"
    if [ -n "$WindowsGcm" ]; then
        HelperCommand="$(BuildWindowsGcmHelperCommand "$WindowsGcm")"
        git config --global --replace-all credential.helper "$HelperCommand" || return 1
        Ok "Configured WSL git to use the Oryn Git Credential Manager bridge."
        Info "GCM path: $WindowsGcm"
        BridgePath="$(git config --global --get credential.helper 2>/dev/null || true)"
        case "$BridgePath" in
            */.oryn/git-credential-manager-wsl-bridge.sh) Info "Bridge script: $BridgePath" ;;
        esac
        GitPathPrefix="$(BuildWindowsGitPathPrefix 2>/dev/null || true)"
        [ -z "$GitPathPrefix" ] || Info "Windows Git PATH bridge: $GitPathPrefix"
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
