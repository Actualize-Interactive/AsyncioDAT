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

# Resolve a Python 3.11 prefix for CMake. Prefer a uv-managed interpreter
# (run `uv python install 3.11` once); fall back to TouchDesigner's bundled
# Python framework if uv is unavailable.
PYTHON_ROOT=""
if command -v uv >/dev/null 2>&1; then
    UV_PYTHON=$(uv python find 3.11 2>/dev/null || true)
    if [ -n "$UV_PYTHON" ]; then
        # <prefix>/bin/python3.11 -> <prefix>
        PYTHON_ROOT=$(dirname "$(dirname "$UV_PYTHON")")
        echo -e "\033[33mUsing uv Python 3.11: $PYTHON_ROOT\033[0m"
    fi
fi

if [ -z "$PYTHON_ROOT" ]; then
    TD_PYTHON="/Applications/TouchDesigner.app/Contents/Frameworks/Python.framework/Versions/Current"
    if [ -d "$TD_PYTHON" ]; then
        PYTHON_ROOT="$TD_PYTHON"
        echo -e "\033[33mUsing TouchDesigner's Python framework: $PYTHON_ROOT\033[0m"
    else
        echo -e "\033[33mNo uv or TouchDesigner Python 3.11 found; relying on CMake default discovery (must be 3.11)\033[0m"
    fi
fi

CMAKE_PY_ARG=()
if [ -n "$PYTHON_ROOT" ]; then
    CMAKE_PY_ARG=(-DPython3_ROOT_DIR="$PYTHON_ROOT")
fi

if [ "$GENERATE_XCODE" = true ]; then
    cmake -G Xcode "${CMAKE_PY_ARG[@]}" ..
else
    cmake "${CMAKE_PY_ARG[@]}" ..
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
