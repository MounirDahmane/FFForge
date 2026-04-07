#!/usr/bin/env bash

set -e
set -x

mkdir -p build
mkdir -p frames

cmake -S . \
    -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    "$@"