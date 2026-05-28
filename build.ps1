# Build AsyncioDAT on Windows.
#
# TouchDesigner embeds CPython 3.11, so we build against a uv-managed Python
# 3.11 (run `uv python install 3.11` once). Python is not vendored; uv supplies
# the headers + import library that CMake needs.

# Store the current location to restore it later
$originalLocation = Get-Location

try {
    # Resolve a Python 3.11 prefix for CMake (find_package(Python3 ... Development.Module)).
    $pythonRoot = $null
    $uvPython = $null
    try { $uvPython = (& uv python find 3.11 2>$null) } catch { }
    if ($uvPython) {
        $pythonRoot = Split-Path -Parent $uvPython
        Write-Host "Using uv Python 3.11: $pythonRoot" -ForegroundColor Yellow
    } else {
        Write-Host "Could not locate Python 3.11 via uv. Run 'uv python install 3.11'." -ForegroundColor Yellow
        Write-Host "Falling back to CMake's default Python discovery (must be 3.11)." -ForegroundColor Yellow
    }

    # Create build directory if it doesn't exist
    if (-not (Test-Path -Path "build")) {
        New-Item -ItemType Directory -Path "build" | Out-Null
        Write-Host "Created build directory" -ForegroundColor Green
    }

    # Configure CMake
    Write-Host "Configuring CMake..." -ForegroundColor Cyan
    Push-Location -Path "build"
    if ($pythonRoot) {
        cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPython3_ROOT_DIR="$pythonRoot"
    } else {
        cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
    }

    # Build the project. Release is the default; pass an argument to override.
    $buildConfig = if ($args[0]) { $args[0] } else { "Release" }
    Write-Host "Building with configuration: $buildConfig" -ForegroundColor Cyan
    cmake --build . --config $buildConfig

    # Return from build directory
    Pop-Location

    Write-Host "Build completed!" -ForegroundColor Green
}
finally {
    # Ensure we always return to the original directory, even if errors occur
    Set-Location -Path $originalLocation
}
