#!/usr/bin/env sh
set -eu

if ! command -v apt-get >/dev/null 2>&1; then
    echo "This helper supports apt-based distributions (Ubuntu/Debian/Raspberry Pi OS)." >&2
    exit 1
fi

if [ "$(id -u)" -eq 0 ]; then
    SUDO=
elif command -v sudo >/dev/null 2>&1; then
    SUDO=sudo
else
    echo "Run as root or install sudo." >&2
    exit 1
fi

$SUDO apt-get update
$SUDO apt-get install -y \
    build-essential cmake git ninja-build pkg-config \
    libasound2-dev libdbus-1-dev libegl1-mesa-dev libgl-dev libgles-dev \
    libudev-dev libx11-dev libxcursor-dev libxext-dev libxi-dev \
    libxinerama-dev libxrandr-dev libxrender-dev libxss-dev libxtst-dev

echo "Linux build dependencies installed. Configure with: cmake --preset linux-release"
