#!/bin/bash
# Minimal host deps for native-test (~few MB). No Poky, no Yocto toolchain.
set -euo pipefail

PACKAGES=(build-essential curl python3 make)

if command -v apt-get >/dev/null 2>&1; then
	if [ "$(id -u)" -ne 0 ]; then
		echo "Install with: sudo $0"
		exit 1
	fi
	export DEBIAN_FRONTEND=noninteractive
	apt-get update
	apt-get install -y "${PACKAGES[@]}"
else
	echo "Install these packages with your package manager: ${PACKAGES[*]}"
	exit 1
fi

echo "Ready. Run: ./native-test/run.sh"
