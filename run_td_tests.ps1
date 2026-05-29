<#
.SYNOPSIS
    TouchDesigner integration test: launch TouchDesigner, run the AsyncioDAT
    test suites inside it, and gate the exit code on the result.

.DESCRIPTION
    TouchDesigner cannot run in cloud CI (it needs a license and a GPU), so this
    is a LOCAL pre-release gate. It launches tests/td/test.toe, which must contain a
    one-time bootstrap Execute DAT that calls td_test_runner.start() on start
    (see TESTING.md). The runner writes results.json and quits TouchDesigner;
    this script waits for that sentinel, parses it, and exits 0 (pass) or 1
    (fail / timeout).

.PARAMETER NoBuild    Skip building; use the operator already in tests/td/Plugins/.
.PARAMETER Toe        Path to the .toe to run. Default: tests/td/test.toe
.PARAMETER OpName     Name of the AsyncioDAT operator in the project. Default: Asyncio1
.PARAMETER TimeoutSec How long to wait for results.json. Default: 180
.PARAMETER SettleFrames Frames the in-TD runner waits for async tasks. Default: 240
.PARAMETER TdPath     Path to TouchDesigner.exe. Default: newest install found.

.EXAMPLE
    .\run_td_tests.ps1            # builds, copies the plugin, runs, reports pass/fail
.EXAMPLE
    .\run_td_tests.ps1 -NoBuild   # reuse the already-built plugin
#>
[CmdletBinding()]
param(
    [switch] $NoBuild,
    [string] $Toe = (Join-Path $PSScriptRoot "tests/td/test.toe"),
    [string] $OpName = "Asyncio1",
    [int]    $TimeoutSec = 180,
    [int]    $SettleFrames = 240,
    [string] $TdPath = ""
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

# Build by default so the operator is always freshly compiled AND copied into
# tests/td/Plugins/ (build.ps1 / CMake POST_BUILD handle the copy). The dev just
# runs this script; no manual build-or-copy step.
if (-not $NoBuild) {
    Write-Host "Building AsyncioDAT (compiles + copies to tests/td/Plugins/)..." -ForegroundColor Cyan
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

if ($summary.success) {
    Write-Host "`nAll integration tests passed." -ForegroundColor Green
    exit 0
} else {
    Write-Host "`nIntegration tests failed." -ForegroundColor Red
    exit 1
}
