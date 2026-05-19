#!/bin/bash
# Lightweight project check — no Poky, no bitbake, no QEMU.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "==> Checking Yocto layer files (structure only)"
required=(
	meta-mysoftware/conf/layer.conf
	meta-mysoftware/recipes-myapps/bootinfo-server/bootinfo-server.bb
	meta-mysoftware/recipes-myapps/bootinfo-server/files/bootinfo-server.c
	meta-mysoftware/recipes-myapps/bootinfo-server/files/bootinfo-server.init
	meta-mysoftware/recipes-core/images/core-image-minimal.bbappend
)
for f in "${required[@]}"; do
	[ -f "$f" ] || { echo "MISSING: $f"; exit 1; }
	echo "  OK $f"
done

echo ""
echo "==> Native build + HTTP smoke test"
./native-test/run.sh

echo ""
echo "==> Disk usage (this repo only)"
if du -sh "$ROOT" 2>/dev/null | awk '{print "  Repo on disk: " $1}'; then
	:
else
	echo "  (du skipped)"
fi
echo ""
echo "All lightweight checks passed."
echo "You do NOT need poky/ or build/ on this machine."
