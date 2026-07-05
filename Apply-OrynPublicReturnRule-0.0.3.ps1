param(
    [string]$SdkRoot = "C:\OrynSDK"
)

$ErrorActionPreference = "Stop"

function Write-Ok($Message) {
    Write-Host "[ OK ] $Message" -ForegroundColor Green
}

function Write-Step($Message) {
    Write-Host "[STEP] $Message" -ForegroundColor Cyan
}

function Write-Fail($Message) {
    Write-Host "[FAIL] $Message" -ForegroundColor Red
}

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$PatchRoot = Join-Path $ScriptRoot "ChangedFiles"
$KernelMainSource = Join-Path $PatchRoot "OrynProjects\Kernel-5\Source\KernelMain.c"
$KernelMainTarget = Join-Path $SdkRoot "OrynProjects\Kernel-5\Source\KernelMain.c"

if (!(Test-Path $KernelMainSource)) {
    Write-Fail "Patch file missing: $KernelMainSource"
    exit 1
}

$targetDirectory = Split-Path -Parent $KernelMainTarget
if (!(Test-Path $targetDirectory)) {
    New-Item -ItemType Directory -Path $targetDirectory -Force | Out-Null
}

if (Test-Path $KernelMainTarget) {
    $backup = "$KernelMainTarget.before-public-return-rule-0.0.3"
    Copy-Item -Path $KernelMainTarget -Destination $backup -Force
    Write-Ok "Backed up existing KernelMain.c to $backup"
}

Copy-Item -Path $KernelMainSource -Destination $KernelMainTarget -Force
Write-Ok "Updated KernelMain.c: $KernelMainTarget"

$StatusHeader = Join-Path $PatchRoot "Common\Kernel\Include\OrynStatus.h"
$StatusSource = Join-Path $PatchRoot "Common\Kernel\Source\Status\OrynStatus.c"

if (Test-Path $StatusHeader) {
    $dest = Join-Path $SdkRoot "Common\Kernel\Include\OrynStatus.h"
    New-Item -ItemType Directory -Path (Split-Path -Parent $dest) -Force | Out-Null
    Copy-Item -Path $StatusHeader -Destination $dest -Force
    Write-Ok "Installed OrynStatus.h"
}

if (Test-Path $StatusSource) {
    $dest = Join-Path $SdkRoot "Common\Kernel\Source\Status\OrynStatus.c"
    New-Item -ItemType Directory -Path (Split-Path -Parent $dest) -Force | Out-Null
    Copy-Item -Path $StatusSource -Destination $dest -Force
    Write-Ok "Installed OrynStatus.c"
}

Write-Step "KernelMain now returns OrynStatus and checks OrynKernelSdkWriteLine."
Write-Step "If the build now fails, the next file to fix is OrynKernelSdkWriteLine/ORYN_KERNEL_APPLICATION so they also use OrynStatus."
