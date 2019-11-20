#!/usr/bin/env bash

set -e # fail script if any individual commands fail

function build_and_run() {
  cmake -E make_directory "$1"
  cmake -E chdir "$1" cmake -DCMAKE_BUILD_TYPE="$2" -DGRPCW_BUILD_TESTS=ON -DGRPCW_USE_DEV_FLAGS=ON ..
  cmake -E chdir "$1" cmake --build . --parallel
}

build_and_run cmake-build-debug Debug
cmake -E chdir cmake-build-debug cmake --build . --target test_grpc_wrapper_coverage --parallel
build_and_run cmake-build-release Release
