#!/usr/bin/env bash
# Clean build artifacts
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "[clean] Removing build directory..."
rm -rf "$ROOT_DIR/build"
echo "[clean] Done."
