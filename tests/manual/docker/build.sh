#!/bin/bash

function build {
    export DOCKER_BUILDKIT=1
    docker build -t "$1" -f Dockerfile-"$1" .
}

build qt-6-ubuntu-24.04-build
build qt-5-ubuntu-16.04-build
build qt-5-ubuntu-20.04-build
build qt-5-ubuntu-20.04-run
build qt-5-ubuntu-20.04-clang-lldb-build
build qt-6-fedora-37-build
