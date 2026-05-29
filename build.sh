#!/usr/bin/env bash
# Build the AsyncioDAT operator and copy it into tests/td/Plugins/.
#
# Thin wrapper over the CMake "dev" preset (see CMakePresets.json). CMake
# auto-detects a uv-managed Python 3.11 (run `uv python install 3.11` once).
# Only the operator target is built here; for the unit tests run:
#   cmake --workflow --preset dev
set -euo pipefail

cd "$(dirname "$0")"
cmake --preset dev
cmake --build --preset dev --target asyncio_dat
echo "Build completed!"
