#!/usr/bin/env bash
# Run performance benchmark
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

CONCURRENCY="${1:-10}"
REQUESTS="${2:-100}"

if [ ! -f "$BUILD_DIR/examples/protobuf_bench/rpcbench" ]; then
    echo "[benchmark] Build not found. Running build first..."
    "$SCRIPT_DIR/build.sh" dev
fi

echo "[benchmark] Running benchmark (concurrency=$CONCURRENCY, requests=$REQUESTS)"
echo "[benchmark] NOTE: Make sure the server is running first:"
echo "  $BUILD_DIR/examples/protobuf_bench/pb_bench_server"
echo ""
read -p "Press Enter after starting the server (or Ctrl+C to cancel)..."

"$BUILD_DIR/examples/protobuf_bench/rpcbench" -c "$CONCURRENCY" -n "$REQUESTS"
