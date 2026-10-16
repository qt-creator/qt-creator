# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# A clean environment for the scripts in scripts/harmonyos, so that what they need is written
# down and what they produce does not depend on the machine they ran on. The image holds
# host tools only. The scripts, the SDK and the installer's Qt are mounted, because their
# licenses are not ours to put into an image, and the image is not meant to be published.
# build_in_docker.py builds the image and runs the scripts in it.

# 26.04 for clang 21, which build_creator.py builds with. Ubuntu 24.04 has 20 at most.
FROM ubuntu:26.04

# zlib1g-dev only for its zlib.pc. The registry's zlib port is empty for OHOS, which takes
# libz from the SDK sysroot, but libpng16.pc still requires zlib, and the SDK ships no .pc
# files. On a desktop host the host's zlib.pc satisfies that check, so this does the same.
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        autoconf autoconf-archive automake bison build-essential ca-certificates clang-21 \
        cmake curl flex git golang-go libtool lld-21 ninja-build pkg-config python3 tar unzip \
        zip zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# What the tools of the installer's desktop Qt link against. The build runs qtpaths.
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        libdbus-1-3 libegl1 libfontconfig1 libfreetype6 libgl1 libglib2.0-0t64 libx11-6 \
        libxkbcommon0 \
    && rm -rf /var/lib/apt/lists/*

# Git refuses to work in a checkout owned by another user, and the output directory is
# owned by whoever runs the container.
RUN git config --system safe.directory '*'
