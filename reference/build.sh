#!/bin/bash

# Store the current location to restore it later
originalLocation=$(pwd)

# Function to restore location on exit
cleanup() {
    cd "$originalLocation"
}

# Set trap to ensure we always return to the original directory
trap cleanup EXIT

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    mkdir -p build
    echo -e "\033[32mCreated build directory\033[0m"
fi

# Configure CMake
echo -e "\033[36mConfiguring CMake...\033[0m"
cd build

# Try to use TouchDesigner's Python first
TD_PYTHON="/Applications/TouchDesigner.app/Contents/Frameworks/Python.framework/Versions/Current/bin/python3.11"
if [ -f "$TD_PYTHON" ]; then
    TD_VERSION=$($TD_PYTHON --version 2>&1 | cut -d' ' -f2)
    echo -e "\033[33mUsing TouchDesigner's Python $TD_VERSION: $TD_PYTHON\033[0m"
    cmake -DPython3_EXECUTABLE="$TD_PYTHON" ..
else
    echo -e "\033[33mTouchDesigner Python not found, using system Python\033[0m"
    cmake ..
fi

# Build the project
echo -e "\033[36mBuilding project...\033[0m"

# Release is the default build configuration
# If an argument is provided, use it as the build configuration
buildConfig=${1:-Release}
echo -e "\033[36mBuilding with configuration: $buildConfig\033[0m"
cmake --build . --config "$buildConfig"

echo -e "\033[32mBuild completed!\033[0m"