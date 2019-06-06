#!/bin/bash
set -e -u -x

# Usage: ./go x86_64-host -DARCH=x86_64 aarch64-host -DARCH=aarch64 ...

run_one() {
    set -e -u -x
    host=$1
    cmake_opts=$2
    rsync -avz --files-from=<(git ls-files) . "$host":bp/
    ssh "$host" bash -c "\"cd bp && cmake -DCMAKE_BUILD_TYPE=Debug $cmake_opts && make && ./bp >bp.bin\""
}
export -f run_one

cd "$(dirname "$0")"
echo "$@" | xargs -n 2 -P "$(getconf _NPROCESSORS_ONLN)" bash -c 'run_one "$@"' --
