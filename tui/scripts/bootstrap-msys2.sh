#!/usr/bin/env sh
set -eu

pacman -S --needed --noconfirm \
    mingw-w64-x86_64-meson \
    mingw-w64-x86_64-ninja \
    mingw-w64-x86_64-gcc \
    mingw-w64-x86_64-pdcurses \
    curl \
    unzip

if [ ! -f /mingw64/lib/libpdcurses.a ]; then
    echo "mingw-w64-x86_64-pdcurses did not install /mingw64/lib/libpdcurses.a" >&2
    echo "Static PDCurses is required for a self-contained scrpeek.exe" >&2
    exit 1
fi
