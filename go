#!/bin/bash
set -e -u -x

# Usage: ./go x86_64-host x86_64 aarch64-host aarch64 ...

run_one() {
    set -e -u -x
    host=$1
    arch=$2
    rsync -avz --files-from=<(git ls-files) . "$host":bp/
    ssh "$host" bash -c "\"cd bp && cmake -DCMAKE_BUILD_TYPE=Debug -DARCH=$arch && make && ./bp >bp.bin\""
    rsync -avz "$host":bp/bp.bin "bp-$arch.bin"
}
export -f run_one

cd "$(dirname "$0")"
echo "$@" | xargs -n 2 -P "$(getconf _NPROCESSORS_ONLN)" bash -c 'run_one "$@"' --
