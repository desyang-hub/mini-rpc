#!/usr/bin/env bash
# Deploy Nacos service registry using Docker Compose
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
NACOS_DIR="${ROOT_DIR}/deploy/nacos"

if ! command -v docker &> /dev/null; then
    echo "[nacos] Docker is not installed. Please install Docker first."
    exit 1
fi

if [ ! -f "$NACOS_DIR/docker-compose.yaml" ]; then
    echo "[nacos] docker-compose.yaml not found at $NACOS_DIR"
    exit 1
fi

ACTION="${1:-up}"

case "$ACTION" in
    up)
        echo "[nacos] Starting Nacos..."
        cd "$NACOS_DIR"
        docker compose up -d
        echo "[nacos] Nacos started. Waiting for readiness..."
        sleep 5
        echo "[nacos] Nacos should be available at http://localhost:8848/nacos"
        echo "[nacos] Default login: nacos/nacos"
        ;;
    down)
        echo "[nacos] Stopping Nacos..."
        cd "$NACOS_DIR"
        docker compose down
        echo "[nacos] Nacos stopped."
        ;;
    status)
        cd "$NACOS_DIR"
        docker compose ps
        ;;
    *)
        echo "Usage: $0 [up|down|status]"
        exit 1
        ;;
esac
