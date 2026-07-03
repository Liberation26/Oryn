NextKernelVariantNumber()
{
    VariantRoot="$1"
    Highest=0

    for Candidate in "$VariantRoot"/[0-9]*; do
        [ -d "$Candidate" ] || continue
        Number="$(basename "$Candidate")"
        case "$Number" in
            *[!0-9]*|'') continue ;;
        esac
        [ "$Number" -gt "$Highest" ] && Highest="$Number"
    done

    printf '%s\n' $((Highest + 1))
}

RewriteMigratedVariantText()
{
    VariantDir="$1"
    Number="$2"
    HeaderPath="$VariantDir/Include/OrynBootInfoSelection.h"
    NotesPath="$VariantDir/BootInfoSelection.txt"

    [ -f "$HeaderPath" ] && sed -i \
        -e "s/#define ORYN_BOOTINFO_SELECTION_NUMBER .*/#define ORYN_BOOTINFO_SELECTION_NUMBER $Number/" \
        -e "s#Kernel-[0-9][0-9]*/Kernel/[0-9][0-9]*#Kernel-5/Kernel/$Number#g" \
        "$HeaderPath"

    [ -f "$NotesPath" ] && sed -i \
        -e "s/Oryn Kernel-[0-9][0-9]* BootInfo selection [0-9][0-9]*/Oryn Kernel-5 BootInfo selection $Number/g" \
        -e "s#Kernel-[0-9][0-9]*/Kernel/[0-9][0-9]*#Kernel-5/Kernel/$Number#g" \
        "$NotesPath"
}

MigrateLegacyKernelBootInfoVariants()
{
    LegacyName="$1"
    LegacyProject="$WorkspaceRoot/OrynProjects/$LegacyName"
    NewProject="$WorkspaceRoot/OrynProjects/Kernel-5"
    LegacyRoot="$LegacyProject/Kernel"
    NewRoot="$NewProject/Kernel"

    [ -d "$LegacyProject" ] || return 0
    [ -d "$LegacyRoot" ] || return 0
    [ -d "$NewRoot" ] || return 0

    LegacySelected=""
    [ -f "$LegacyRoot/Selected.txt" ] && LegacySelected="$(tr -d '\r\n' < "$LegacyRoot/Selected.txt")"
    NewSelected=""

    for LegacyVariant in "$LegacyRoot"/[0-9]*; do
        [ -d "$LegacyVariant" ] || continue
        LegacyNumber="$(basename "$LegacyVariant")"
        LegacyMask="$(GetBootInfoVariantMask "$LegacyVariant" 2>/dev/null || true)"
        [ -n "$LegacyMask" ] || continue

        ExistingNumber="$(FindVariantByMask "$NewRoot" "$LegacyMask" 2>/dev/null || true)"
        if [ -n "$ExistingNumber" ]; then
            [ "$LegacyNumber" = "$LegacySelected" ] && NewSelected="$ExistingNumber"
            continue
        fi

        NewNumber="$(NextKernelVariantNumber "$NewRoot")"
        cp -a "$LegacyVariant" "$NewRoot/$NewNumber"
        RewriteMigratedVariantText "$NewRoot/$NewNumber" "$NewNumber"
        Ok "Migrated $LegacyName BootInfo variant Kernel/$LegacyNumber to Kernel-5/Kernel/$NewNumber."
        [ "$LegacyNumber" = "$LegacySelected" ] && NewSelected="$NewNumber"
    done

    if [ -n "$NewSelected" ]; then
        printf '%s\n' "$NewSelected" > "$NewRoot/Selected.txt"
        Ok "Preserved selected BootInfo variant as Kernel-5/Kernel/$NewSelected."
    fi
}

RemoveLegacyProjectIfGenerated()
{
    LegacyProject="$1"
    LegacyName="$2"

    [ -f "$DefaultProject" ] || return 0
    [ -d "$LegacyProject" ] || return 0
    [ -f "$LegacyProject/Project.oryn" ] || return 0

    if grep -qx "Name=$LegacyName" "$LegacyProject/Project.oryn" 2>/dev/null; then
        rm -rf "$LegacyProject"
        Ok "Removed old generated $LegacyName project folder. Kernel-5 is now the default project."
    fi
}


RemoveObsoleteSdkSourcePaths()
{
    for ObsoletePath in         "$SdkRoot/Source"         "$SdkRoot/Bin"         "$SdkRoot/Docs"         "$SdkRoot/Scripts"         "$SdkRoot/Common/Boot"; do
        if [ -e "$ObsoletePath" ]; then
            rm -rf "$ObsoletePath"
            Ok "Removed obsolete SDK path: ${ObsoletePath#$SdkRoot/}"
        fi
    done
}


RemoveMovedKernelFile()
{
    MovedPath="$1"

    if [ -e "$MovedPath" ]; then
        rm -rf "$MovedPath"
        Ok "Removed project-owned copy now supplied by SDK: ${MovedPath#$WorkspaceRoot/}"
    fi
}


RemoveMovedSharedKernelProjectPaths()
{
    KernelProject="$WorkspaceRoot/OrynProjects/Kernel-5"
    [ -d "$KernelProject" ] || return 0

    RemoveMovedKernelFile "$KernelProject/Include/KernelBootInfo.h"
    RemoveMovedKernelFile "$KernelProject/Include/KernelConsole.h"
    RemoveMovedKernelFile "$KernelProject/Include/KernelIo.h"
    RemoveMovedKernelFile "$KernelProject/Include/KernelMemoryMap.h"
    RemoveMovedKernelFile "$KernelProject/Include/KernelPhysicalMemory.h"
    RemoveMovedKernelFile "$KernelProject/Include/KernelTtf.h"
    RemoveMovedKernelFile "$KernelProject/Include/KernelVirtualMemory.h"
    RemoveMovedKernelFile "$KernelProject/Include/Serial.h"

    RemoveMovedKernelFile "$KernelProject/Source/BootInfo"
    RemoveMovedKernelFile "$KernelProject/Source/Console"
    RemoveMovedKernelFile "$KernelProject/Source/Fonts"
    RemoveMovedKernelFile "$KernelProject/Source/KernelIo.c"
    RemoveMovedKernelFile "$KernelProject/Source/Serial.c"
    RemoveMovedKernelFile "$KernelProject/Source/Memory/KernelMemoryMap.c"
    RemoveMovedKernelFile "$KernelProject/Source/Memory/KernelMemoryMapPrint.c"
    RemoveMovedKernelFile "$KernelProject/Source/Memory/KernelPhysicalMemory.c"
    RemoveMovedKernelFile "$KernelProject/Source/Memory/KernelPhysicalMemoryPrint.c"
    RemoveMovedKernelFile "$KernelProject/Source/Memory/KernelVirtualMemory.c"
    RemoveMovedKernelFile "$KernelProject/Source/Memory/KernelVirtualMemoryPrint.c"
    find "$KernelProject/Source" -type d -empty -delete 2>/dev/null || true
    find "$KernelProject/Include" -type d -empty -delete 2>/dev/null || true
}


MigrateLegacyKernelBootInfoVariants "Kernel-3"
MigrateLegacyKernelBootInfoVariants "Kernel-4"
RemoveObsoleteSdkSourcePaths
RemoveMovedSharedKernelProjectPaths
RemoveLegacyProjectIfGenerated "$WorkspaceRoot/OrynProjects/TestOS" "TestOS"
RemoveLegacyProjectIfGenerated "$WorkspaceRoot/OrynProjects/Kernel-1" "Kernel-1"
RemoveLegacyProjectIfGenerated "$WorkspaceRoot/OrynProjects/Kernel-2" "Kernel-2"
RemoveLegacyProjectIfGenerated "$WorkspaceRoot/OrynProjects/Kernel-3" "Kernel-3"
RemoveLegacyProjectIfGenerated "$WorkspaceRoot/OrynProjects/Kernel-4" "Kernel-4"

if [ "$#" -gt 0 ]; then
    case "$1" in
        GitPush|gitpush|push)
            shift || true
            exec bash "$SdkRoot/Common/Scripts/GitPushOryn.sh" "$@"
            ;;
        GitLogin|gitlogin|gcm|credentials)
            shift || true
            exec bash "$SdkRoot/Common/Scripts/GitPushOryn.sh" --setup-gcm-only "$@"
            ;;
    esac
fi

NeedBuild=0
if [ ! -x "$OrynBin" ]; then
    NeedBuild=1
elif find "$SdkRoot/Common/OrynBuild" -type f \( -name '*.c' -o -name '*.h' \) -newer "$OrynBin" 2>/dev/null | grep -q .; then
    NeedBuild=1
elif find "$SdkRoot/Targets/UEFI/X64/OrynBuild" -type f \( -name '*.c' -o -name '*.h' \) -newer "$OrynBin" 2>/dev/null | grep -q .; then
    NeedBuild=1
elif [ -f "$SdkRoot/VERSION" ]; then
    ExpectedVersion="$(tr -d '\r\n' < "$SdkRoot/VERSION")"
    CurrentVersion="$("$OrynBin" version 2>/dev/null | awk '{print $4}' | head -n 1)"
    if [ "$ExpectedVersion" != "$CurrentVersion" ]; then
        NeedBuild=1
    fi
fi

if [ "$NeedBuild" -eq 1 ]; then
    if [ ! -f "$BuildScript" ]; then
        Fail "Build script was not found: $BuildScript"
        exit 1
    fi

    Info "Bootstrapping native Oryn build tool."
    bash "$BuildScript" || exit $?
fi

if [ ! -x "$OrynBin" ]; then
    Fail "Native Oryn build tool was not produced: $OrynBin"
    exit 1
fi

RunDefaultProject()
{
    if [ ! -f "$DefaultProject" ]; then
        Fail "Default Kernel-5 project was not found: $DefaultProject"
        Warn "Expected workspace layout:"
        Warn "  $WorkspaceRoot/OrynSDK"
        Warn "  $WorkspaceRoot/OrynProjects/Kernel-5/Project.oryn"
        Warn "Use: ./Oryn.sh run /path/to/Project.oryn"
        exit 1
    fi

    exec "$OrynBin" run "$DefaultProject"
}

if [ "$#" -eq 0 ]; then
    RunDefaultProject
fi

Command="$1"
shift || true

case "$Command" in
    rebuild|bootstrap)
        exec bash "$BuildScript"
        ;;

    doctor|version)
        exec "$OrynBin" "$Command" "$@"
        ;;

    build|image|run|matrix|matrix-serial|matrix-screen|matrix-all|clean)
        if [ "$#" -eq 0 ]; then
            if [ ! -f "$DefaultProject" ]; then
                Fail "Default Kernel-5 project was not found: $DefaultProject"
                Warn "Expected workspace layout:"
                Warn "  $WorkspaceRoot/OrynSDK"
                Warn "  $WorkspaceRoot/OrynProjects/Kernel-5/Project.oryn"
                Warn "Use: ./Oryn.sh $Command /path/to/Project.oryn"
                exit 1
            fi
            exec "$OrynBin" "$Command" "$DefaultProject"
        fi
        exec "$OrynBin" "$Command" "$@"
        ;;

    OSName|osname|name)
        if [ "$#" -eq 0 ]; then
            RunOSNameQuestionnaire "$DefaultProject"
        fi

        case "$1" in
            *.oryn)
                ProjectFile="$1"
                shift || true
                RunOSNameQuestionnaire "$ProjectFile" "$@"
                ;;
            *)
                RunOSNameQuestionnaire "$DefaultProject" "$@"
                ;;
        esac
        ;;

    VMSettings|vmsettings|VM|vm)
        if [ "$#" -eq 0 ]; then
            RunVMSettingsQuestionnaire "$DefaultProject"
            exit 0
        fi

        case "$1" in
            *.oryn)
                ProjectFile="$1"
                shift || true
                RunVMSettingsQuestionnaire "$ProjectFile" "$@"
                exit 0
                ;;
            *)
                RunVMSettingsQuestionnaire "$DefaultProject" "$@"
                exit 0
                ;;
        esac
        ;;

    Headless|headless)
        if [ "$#" -eq 0 ]; then
            RunHeadlessQuestionnaire "$DefaultProject"
        fi

        case "$1" in
            *.oryn)
                ProjectFile="$1"
                shift || true
                RunHeadlessQuestionnaire "$ProjectFile" "$@"
                ;;
            *)
                RunHeadlessQuestionnaire "$DefaultProject" "$@"
                ;;
        esac
        ;;


    BootConfig|bootconfig|CommandLine|commandline)
        if [ "$#" -eq 0 ]; then
            RunBootConfigQuestionnaire "$DefaultProject"
        fi

        case "$1" in
            *.oryn)
                ProjectFile="$1"
                shift || true
                RunBootConfigQuestionnaire "$ProjectFile" "$@"
                ;;
            *)
                RunBootConfigQuestionnaire "$DefaultProject" "$@"
                ;;
        esac
        ;;

    bootinfo)
        if [ ! -f "$DefaultProject" ]; then
            Fail "Default Kernel-5 project was not found: $DefaultProject"
            Warn "Expected workspace layout:"
            Warn "  $WorkspaceRoot/OrynSDK"
            Warn "  $WorkspaceRoot/OrynProjects/Kernel-5/Project.oryn"
            Warn "Use: ./Oryn.sh bootinfo /path/to/Project.oryn"
            exit 1
        fi

        if [ "$#" -eq 0 ]; then
            exec "$OrynBin" bootinfo "$DefaultProject"
        fi

        case "$1" in
            *.oryn)
                exec "$OrynBin" bootinfo "$@"
                ;;
            *)
                exec "$OrynBin" bootinfo "$DefaultProject" "$@"
                ;;
        esac
        ;;

    help|--help|-h)
        printf 'Oryn WSL SDK launcher\n\n'
        printf 'Workspace root:\n'
        printf '  %s\n\n' "$WorkspaceRoot"
        printf 'SDK root:\n'
        printf '  %s\n\n' "$SdkRoot"
        printf 'Default project:\n'
        printf '  %s\n\n' "$DefaultProject"
        printf 'Usage:\n'
        printf '  ./Oryn.sh                 Build, image, and run Kernel-5\n'
        printf '  ./Oryn.sh doctor          Check tools\n'
        printf '  ./Oryn.sh version         Show SDK version\n'
        printf '  ./Oryn.sh build [project] Build project, defaulting to Kernel-5\n'
        printf '  ./Oryn.sh image [project] Build image, defaulting to Kernel-5\n'
        printf '  ./Oryn.sh run [project]   Run project, defaulting to Kernel-5\n'
        printf '  ./Oryn.sh matrix [project]        Build every profile and test headless serial QEMU\n'
        printf '  ./Oryn.sh matrix-screen [project] Build every profile and test graphical framebuffer screen QEMU\n'
        printf '  ./Oryn.sh matrix-all [project]    Run serial matrix, then graphical screen matrix\n'
        printf '  ./Oryn.sh OSName [project]   Ask kernel/OS name first, then Headless, then BootInfo\n'
        printf '  ./Oryn.sh VMSettings [project] Ask and save VM format, CPU, memory, PIC/APIC/APIC2/HPET settings\n'
        printf '  ./Oryn.sh Headless [project] Ask whether the VM is headless, save Display, then ask BootInfo\n'
        printf '  ./Oryn.sh clean [project] Clean project, defaulting to Kernel-5\n'
        printf '  ./Oryn.sh bootinfo [project] Ask which UEFI BootInfo items to pass, then build and run\n'
        printf '  ./Oryn.sh bootinfo list      List Kernel-5 BootInfo variants\n'
        printf '  ./Oryn.sh bootinfo show [n]  Show selected or numbered variant\n'
        printf '  ./Oryn.sh bootinfo select n  Select an existing variant\n'
        printf '  ./Oryn.sh bootinfo compare a b Compare two variants\n'
        printf '  ./Oryn.sh bootinfo run n     Select, build, image, and run a variant\n'
        printf '  ./Oryn.sh bootinfo test-all  Build, image, and run every variant\n'
        printf '  ./Oryn.sh gitpush         Initialise git, ignore output, commit, and push source\n'
        printf '  ./Oryn.sh gitlogin        Configure Git Credential Manager for WSL GitHub login\n'
        printf '  ./Oryn.sh rebuild         Rebuild Common/Bin/oryn\n'
        exit 0
        ;;

    *.oryn)
        exec "$OrynBin" run "$Command" "$@"
        ;;

    *)
        exec "$OrynBin" "$Command" "$@"
        ;;
esac
