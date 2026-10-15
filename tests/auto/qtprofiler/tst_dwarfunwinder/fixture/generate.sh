#!/bin/sh
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Builds fixture.c for x86_64 and aarch64 Linux, runs it, and keeps for each
#   fixture-<arch>          the executable, not position-independent, so that
#                           it is mapped at the addresses it was linked for,
#                           and with 4 KiB pages, which keeps it small
#   fixture-<arch>.sample   what it wrote: the registers and the stack
#   fixture-<arch>.symbols  "nm -S" of the executable
# Needs docker, and either machine type or emulation for the other.

set -e
cd "$(dirname "$0")"
for arch in x86_64 aarch64; do
    case $arch in
        x86_64) platform=linux/amd64 ;;
        aarch64) platform=linux/arm64 ;;
    esac
    docker run --rm --platform $platform -v "$PWD:/fixture" -w /fixture debian:stable-slim sh -c "
        apt-get update -qq && apt-get install -qq -y gcc binutils >/dev/null &&
        gcc -std=gnu11 -O2 -g -fomit-frame-pointer -fasynchronous-unwind-tables -no-pie \
            -Wl,-z,max-page-size=4096 -o fixture-$arch fixture.c -lpthread &&
        ./fixture-$arch fixture-$arch.sample &&
        nm -S --defined-only fixture-$arch | grep -E ' [tT] (leaf|level[123]|main)\$' \
            > fixture-$arch.symbols"
done
