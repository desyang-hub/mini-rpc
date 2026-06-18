#!/usr/bin/env bash
# One-click build script for mini-rpc
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

MODE="${1:-dev}"  # dev | release

case "$MODE" in
    dev)
        echo "[build] Development build (examples + tests)"
        cmake -B "$ROOT_DIR/build" \
            -DCMAKE_BUILD_TYPE=Debug \
            -DBUILD_EXAMPLES=ON \
            -DBUILD_TESTS=ON \
            "$ROOT_DIR"
        cmake --build "$ROOT_DIR/build" -j"$(nproc)"
        ;;
    release)
        echo "[build] Release build (no examples, no tests)"
        cmake -B "$ROOT_DIR/build" \
            -DCMAKE_BUILD_TYPE=Release \
            -DBUILD_EXAMPLES=OFF \
            -DBUILD_TESTS=OFF \
            "$ROOT_DIR"
        cmake --build "$ROOT_DIR/build" --config Release -j"$(nproc)"
        ;;
    test)
        echo "[build] Test build"
        cmake -B "$ROOT_DIR/build" \
            -DCMAKE_BUILD_TYPE=Debug \
            -DBUILD_EXAMPLES=OFF \
            -DBUILD_TESTS=ON \
            "$ROOT_DIR"
        cmake --build "$ROOT_DIR/build" -j"$(nproc)"
        ;;
    *)
        echo "Usage: $0 [dev|release|test]"
        exit 1
        ;;
esac

echo "[build] Done: $MODE"
