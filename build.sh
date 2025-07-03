#!/bin/bash

# Store the current location to restore it later
originalLocation=$(pwd)

# Function to restore location on exit
cleanup() {
    cd "$originalLocation"
}

# Set trap to ensure we always return to the original directory
trap cleanup EXIT

# Parse command line arguments
GENERATE_XCODE=false
buildConfig="Release"

for arg in "$@"; do
    case $arg in
        -x|--xcode)
            GENERATE_XCODE=true
            shift
            ;;
        *)
            buildConfig="$arg"
            ;;
    esac
done

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
    
    if [ "$GENERATE_XCODE" = true ]; then
        cmake -G Xcode -DPython3_EXECUTABLE="$TD_PYTHON" ..
    else
        cmake -DPython3_EXECUTABLE="$TD_PYTHON" ..
    fi
else
    echo -e "\033[33mTouchDesigner Python not found, using system Python\033[0m"
    
    if [ "$GENERATE_XCODE" = true ]; then
        cmake -G Xcode ..
    else
        cmake ..
    fi
fi

if [ "$GENERATE_XCODE" = true ]; then
    # Check if Xcode project was generated
    XCODE_PROJECT=$(find . -name "*.xcodeproj" -type d | head -n 1)
    
    if [ -n "$XCODE_PROJECT" ]; then
        echo -e "\033[32mXcode project generated: $(basename "$XCODE_PROJECT")\033[0m"
        echo -e "\033[36mYou can now open the project in Xcode:\033[0m"
        echo -e "\033[33mopen $(pwd)/$(basename "$XCODE_PROJECT")\033[0m"
    else
        echo -e "\033[31mError: Xcode project was not generated\033[0m"
        exit 1
    fi
else
    # Build the project
    echo -e "\033[36mBuilding project...\033[0m"
    echo -e "\033[36mBuilding with configuration: $buildConfig\033[0m"
    cmake --build . --config "$buildConfig"
fi

echo -e "\033[32mBuild completed!\033[0m"
