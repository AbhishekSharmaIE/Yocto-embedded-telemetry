#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"

make -q bootinfo-server 2>/dev/null || make

./bootinfo-server &
PID=$!
trap 'kill "$PID" 2>/dev/null || true' EXIT

sleep 0.5
curl -sf http://127.0.0.1:8000/bootinfo | python3 -m json.tool
echo "OK: /bootinfo returned valid JSON"
