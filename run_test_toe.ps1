# test/test.toe

# This script is used to run the test.toe file in TouchDesigner.
# It sets the path to the TouchDesigner executable and the .toe file, then runs the .toe file using the TouchDesigner executable.


# get the script directory
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition

# get the root directory
$rootDir = $scriptDir

# run ./build.ps1
$buildScriptPath = Join-Path $rootDir "build.ps1"
if (Test-Path $buildScriptPath) {
    $buildConfig = if ($args[0]) { $args[0] } else { "Release" }
    Write-Host "Building with configuration: $buildConfig" -ForegroundColor Cyan
    & $buildScriptPath $buildConfig
} else {
    Write-Host "Error: The build script does not exist at the specified path." -ForegroundColor Red
}

# Set the path to the TouchDesigner executable
$touchDesignerPath = "C:\Program Files\Derivative\TouchDesigner\bin\TouchDesigner.exe"
# Set the path to the .toe file
$toeFilePath = Join-Path $scriptDir "./test/test.toe"
# Check if the TouchDesigner executable exists
if (Test-Path $touchDesignerPath) {
    # Check if the .toe file exists
    if (Test-Path $toeFilePath) {
        # Run the .toe file using the TouchDesigner executable
        Start-Process -FilePath $touchDesignerPath -ArgumentList $toeFilePath
    } else {
        Write-Host "Error: The .toe file does not exist at the specified path." -ForegroundColor Red
    }
} else {
    Write-Host "Error: The TouchDesigner executable does not exist at the specified path." -ForegroundColor Red
}

