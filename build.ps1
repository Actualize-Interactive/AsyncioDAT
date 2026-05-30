# Build the AsyncioDAT operator and copy it into tests/td/Plugins/.
#
# Thin wrapper over the CMake "dev" preset (see CMakePresets.json). CMake
# auto-detects a uv-managed Python 3.11 (run `uv python install 3.11` once).
# Only the operator target is built here; for the unit tests run:
#   cmake --workflow --preset dev
$ErrorActionPreference = "Stop"

Push-Location $PSScriptRoot
try {
    cmake --preset dev
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }
    cmake --build --preset dev --target asyncio_dat
    if ($LASTEXITCODE -ne 0) { throw "Build failed." }
    Write-Host "Build completed!" -ForegroundColor Green
}
finally {
    Pop-Location
}
