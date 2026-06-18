#!/usr/bin/env bash
# Run all unit tests
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

if [ ! -d "$BUILD_DIR/tests" ]; then
    echo "[test] Build not found. Running build first..."
    "$SCRIPT_DIR/build.sh" test
fi

echo "[test] Running all tests..."
cd "$BUILD_DIR"
ctest --output-on-failure -j"$(nproc)"

echo "[test] All tests passed."
