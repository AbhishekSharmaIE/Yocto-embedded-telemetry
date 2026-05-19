#!/bin/bash
# OPTIONAL — large Yocto host package set (~hundreds of MB apt).
# For daily work on a small disk, use scripts/setup-native.sh instead.
# Only run this on a cloud VM if you plan bitbake + QEMU (see docs/expected-build-output.md).
set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
	echo "Run with sudo: sudo $0"
	exit 1
fi

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y \
	gawk wget git diffstat unzip texinfo gcc build-essential \
	chrpath socat cpio python3 python3-pip python3-pexpect \
	xz-utils debianutils iputils-ping python3-git python3-jinja2 \
	libegl1-mesa libsdl1.2-dev xterm python3-subunit \
	mesa-common-dev zstd liblz4-tool file curl

echo "Host prerequisites installed."
