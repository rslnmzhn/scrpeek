#!/usr/bin/env sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
next_version="$script_dir/../scripts/next-version.sh"

check_next() {
    input=$1
    expected=$2
    actual=$("$next_version" "$input")
    if [ "$actual" != "$expected" ]; then
        echo "next-version.sh $input: expected $expected, got $actual" >&2
        exit 1
    fi
}

check_next 1.0.0 1.0.1
check_next 1.0.9 1.1.0
check_next 1.9.9 2.0.0
