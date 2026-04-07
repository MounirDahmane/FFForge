#!/usr/bin/env bash

set -e
set -x

cmake --build build

./build/ffforge "$@"