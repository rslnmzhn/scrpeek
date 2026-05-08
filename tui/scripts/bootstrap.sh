#!/usr/bin/env sh
set -eu

has_cmd() {
    command -v "$1" >/dev/null 2>&1
}

install_apt() {
    sudo apt-get update
    sudo apt-get install -y ninja-build meson libncurses-dev
}

install_brew() {
    brew update
    brew install ninja meson ncurses
}

install_choco() {
    choco install -y ninja meson
}

if has_cmd apt-get; then
    install_apt
elif has_cmd brew; then
    install_brew
elif has_cmd choco; then
    install_choco
else
    echo "No supported package manager found (apt, brew, choco)" >&2
    exit 1
fi

has_cmd ninja || has_cmd ninja-build || {
    echo "ninja was not installed or is not on PATH" >&2
    exit 1
}

has_cmd meson || {
    echo "meson was not installed or is not on PATH" >&2
    exit 1
}
