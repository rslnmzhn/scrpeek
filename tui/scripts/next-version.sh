#!/usr/bin/env sh
set -eu

current=${1:-}
if [ -z "$current" ]; then
    echo "1.0.0"
    exit 0
fi

case "$current" in
    v*) current=${current#v} ;;
esac

old_ifs=$IFS
IFS=.
set -- $current
IFS=$old_ifs

if [ "$#" -ne 3 ]; then
    echo "1.0.0"
    exit 0
fi

x=$1
y=$2
z=$3

case "$x:$y:$z" in
    *[!0-9:]*|:*|*:|*::* )
        echo "1.0.0"
        exit 0
        ;;
esac

z=$((z + 1))
if [ "$z" -ge 10 ]; then
    z=0
    y=$((y + 1))
fi
if [ "$y" -ge 10 ]; then
    y=0
    x=$((x + 1))
fi

echo "$x.$y.$z"
