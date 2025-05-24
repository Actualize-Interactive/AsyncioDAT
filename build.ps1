# Store the current location to restore it later
$originalLocation = Get-Location

try {
    # Create build directory if it doesn't exist
    if (-not (Test-Path -Path "build")) {
        New-Item -ItemType Directory -Path "build" | Out-Null
        Write-Host "Created build directory" -ForegroundColor Green
    }

    # Configure CMake (using Push-Location instead of Set-Location)
    Write-Host "Configuring CMake..." -ForegroundColor Cyan
    Push-Location -Path "build"
    cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo

    # Build the project
    Write-Host "Building project..." -ForegroundColor Cyan
    
    # Release is the default build configuration
    # If an argument is provided, use it as the build configuration
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