param(
    [Parameter(Mandatory=$true)]
    [string]$SdkRoot
)

$ErrorActionPreference = "Stop"

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$changedFiles = Join-Path $scriptRoot "ChangedFiles"

if (!(Test-Path $changedFiles)) {
    throw "ChangedFiles folder was not found beside this script."
}

if (!(Test-Path $SdkRoot)) {
    throw "SDK root was not found: $SdkRoot"
}

Write-Host "[STEP] Applying Oryn public return rule 0.0.2" -ForegroundColor Cyan
Write-Host "[STEP] SDK root: $SdkRoot" -ForegroundColor Cyan

Copy-Item -Path (Join-Path $changedFiles "*") -Destination $SdkRoot -Recurse -Force

$patchSdk = Join-Path $SdkRoot "Common\SDK\Tools\OrynPatchKernelSdkReturnRule.py"
$fixKernelMain = Join-Path $SdkRoot "Common\SDK\Tools\OrynFixKernelMainReturn.py"

if (Get-Command python -ErrorAction SilentlyContinue) {
    Write-Host "[STEP] Patching OrynKernelSdk declarations and write functions" -ForegroundColor Cyan
    python $patchSdk $SdkRoot

    Write-Host "[STEP] Converting KernelMain void entry points" -ForegroundColor Cyan
    python $fixKernelMain $SdkRoot
} else {
    Write-Host "[WARN] python was not found. Files were copied, but automatic patching was not run." -ForegroundColor Yellow
    Write-Host "[WARN] Run the two Python tools manually after installing Python." -ForegroundColor Yellow
}

Write-Host "[ OK ] Oryn public return rule 0.0.2 applied." -ForegroundColor Green
