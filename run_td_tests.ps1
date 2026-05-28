<#
.SYNOPSIS
    Tier 2 integration test: launch TouchDesigner, run the AsyncioDAT test
    suites inside it, and gate the exit code on the result.

.DESCRIPTION
    TouchDesigner cannot run in cloud CI (it needs a license and a GPU), so this
    is a LOCAL pre-release gate. It launches test/test.toe, which must contain a
    one-time bootstrap Execute DAT that calls td_test_runner.start() on start
    (see TESTING.md). The runner writes results.json and quits TouchDesigner;
    this script waits for that sentinel, parses it, and exits 0 (pass) or 1
    (fail / timeout).

.PARAMETER Build      Build the operator first (build.ps1) before launching.
.PARAMETER Toe        Path to the .toe to run. Default: test/test.toe
.PARAMETER OpName     Name of the AsyncioDAT operator in the project. Default: Asyncio1
.PARAMETER TimeoutSec How long to wait for results.json. Default: 180
.PARAMETER SettleFrames Frames the in-TD runner waits for async tasks. Default: 240
.PARAMETER TdPath     Path to TouchDesigner.exe. Default: newest install found.
.PARAMETER Grpc       Also run the external gRPC client test while TD is up.

.EXAMPLE
    .\run_td_tests.ps1 -Build
#>
[CmdletBinding()]
param(
    [switch] $Build,
    [string] $Toe = (Join-Path $PSScriptRoot "test/test.toe"),
    [string] $OpName = "Asyncio1",
    [int]    $TimeoutSec = 180,
    [int]    $SettleFrames = 240,
    [string] $TdPath = "",
    [switch] $Grpc
)

$ErrorActionPreference = "Stop"

function Resolve-TouchDesigner {
    param([string] $Explicit)
    if ($Explicit) {
        if (Test-Path $Explicit) { return $Explicit }
        throw "TouchDesigner.exe not found at: $Explicit"
    }
    if ($env:ASYNCIODAT_TD -and (Test-Path $env:ASYNCIODAT_TD)) { return $env:ASYNCIODAT_TD }
    $candidates = Get-ChildItem "C:/Program Files/Derivative/TouchDesigner*/bin/TouchDesigner.exe" -ErrorAction SilentlyContinue |
        Sort-Object FullName -Descending
    if ($candidates) { return $candidates[0].FullName }
    throw "Could not find TouchDesigner.exe. Pass -TdPath or set ASYNCIODAT_TD."
}

if ($Build) {
    Write-Host "Building AsyncioDAT..." -ForegroundColor Cyan
    & (Join-Path $PSScriptRoot "build.ps1")
    if ($LASTEXITCODE -ne 0) { throw "Build failed." }
}

$toeFull = (Resolve-Path $Toe).Path
$td = Resolve-TouchDesigner -Explicit $TdPath
$resultsPath = Join-Path (Split-Path -Parent $toeFull) "results.json"

Write-Host "TouchDesigner : $td"      -ForegroundColor DarkGray
Write-Host "Project       : $toeFull" -ForegroundColor DarkGray
Write-Host "Results file  : $resultsPath" -ForegroundColor DarkGray

# Clear any stale results so we only ever read this run's output.
if (Test-Path $resultsPath) { Remove-Item $resultsPath -Force }

# The in-TD runner reads these (inherited by the child process).
$env:ASYNCIODAT_RESULTS = $resultsPath
$env:ASYNCIODAT_OP = $OpName
$env:ASYNCIODAT_SETTLE_FRAMES = "$SettleFrames"

Write-Host "Launching TouchDesigner..." -ForegroundColor Cyan
$proc = Start-Process -FilePath $td -ArgumentList "`"$toeFull`"" -PassThru

$grpcJob = $null
if ($Grpc) {
    # Best-effort: exercise the gRPC server (started by on_start) from outside TD.
    $grpcJob = Start-Job -ScriptBlock {
        param($dir)
        Set-Location $dir
        Start-Sleep -Seconds 5
        uv run python test_grpc_service.py 2>&1
    } -ArgumentList (Split-Path -Parent $toeFull)
}

# Wait for the sentinel or timeout.
$deadline = (Get-Date).AddSeconds($TimeoutSec)
$found = $false
while ((Get-Date) -lt $deadline) {
    if (Test-Path $resultsPath) { $found = $true; break }
    if ($proc.HasExited -and -not (Test-Path $resultsPath)) {
        Start-Sleep -Milliseconds 500   # let a last-moment write flush
        if (Test-Path $resultsPath) { $found = $true }
        break
    }
    Start-Sleep -Milliseconds 500
}

# Make sure TouchDesigner is gone (it should self-quit via project.quit).
if (-not $proc.HasExited) {
    Write-Host "Stopping TouchDesigner..." -ForegroundColor DarkGray
    try { Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue } catch {}
}

$grpcOutput = $null
if ($grpcJob) {
    $grpcOutput = Receive-Job $grpcJob -Wait -AutoRemoveJob
}

if (-not $found) {
    Write-Host ""
    Write-Host "FAILED: no results.json after $TimeoutSec s." -ForegroundColor Red
    Write-Host "Is test.toe wired with the bootstrap Execute DAT? See TESTING.md." -ForegroundColor Yellow
    exit 1
}

$summary = Get-Content $resultsPath -Raw | ConvertFrom-Json
Write-Host ""
Write-Host "==== AsyncioDAT integration results ====" -ForegroundColor Cyan
foreach ($r in $summary.results) {
    $tag = if ($r.passed) { "PASS" } else { "FAIL" }
    $color = if ($r.passed) { "Green" } else { "Red" }
    Write-Host ("  [{0}] {1} {2}" -f $tag, $r.name, $r.detail) -ForegroundColor $color
}
Write-Host ("Total: {0} passed, {1} failed" -f $summary.passed, $summary.failed) -ForegroundColor Cyan

if ($Grpc -and $grpcOutput) {
    Write-Host ""
    Write-Host "==== gRPC client output ====" -ForegroundColor Cyan
    $grpcOutput | ForEach-Object { Write-Host "  $_" }
}

if ($summary.success) {
    Write-Host "`nAll integration tests passed." -ForegroundColor Green
    exit 0
} else {
    Write-Host "`nIntegration tests failed." -ForegroundColor Red
    exit 1
}
