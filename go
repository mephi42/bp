#!/bin/bash
set -e -u -x

run_one() {
    set -e -u -x
    host=$1
    rsync -avz --files-from=<(git ls-files) . "$host":bp/
    ssh "$host" bash -c "\"cd bp && cmake -DCMAKE_BUILD_TYPE=Debug && make && ./bp >bp.bin\""
}
export -f run_one

cd "$(dirname "$0")"
echo "$@" | xargs -n 1 -P "$(getconf _NPROCESSORS_ONLN)" bash -c 'run_one "$@"' --
