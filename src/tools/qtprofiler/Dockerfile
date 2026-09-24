# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# A Qt for WebAssembly configured the way README.md next to this file describes,
# and everything else it takes to build qtprofiler against it: single-threaded,
# JSPI, Qt built for size, and only the parts of Qt that qtprofiler links.
#
# Build the image. Its context is this directory, for the patches in qt-patches:
#
#     docker build -t qtprofiler-wasm src/tools/qtprofiler
#
# Build qtprofiler from a Qt Creator checkout. The build directory is
# builds/wasm-docker in that checkout, so a second run is incremental, and the
# files to serve end up in builds/wasm-docker/dist:
#
#     docker run --rm -v "$PWD:/src" qtprofiler-wasm
#
# Arguments after the image name go to scripts/build-wasm-profiler.py. On Linux,
# add --user "$(id -u):$(id -g)" so that the build is not owned by root.

ARG QT_VERSION=6.11.2
ARG EMSDK_VERSION=4.0.7

# binaryen's tools, wasm-opt among them, need libatomic. A compiler brings it in,
# but the image the application is built in has no compiler of its own.
FROM ubuntu:24.04 AS runtime
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        ca-certificates cmake curl fonttools git libatomic1 ninja-build python3 xz-utils \
    && rm -rf /var/lib/apt/lists/*

FROM runtime AS build
ARG QT_VERSION
ARG EMSDK_VERSION
# Checksums of the release tarballs, from download.qt.io. Pass new ones together
# with a new QT_VERSION.
ARG QTBASE_SHA256=5b2e00eccaf5a4d8c14134ffa0ea8dfd0a35ae1ffc7f8d87fa4305a1ed23cf22
ARG QTSVG_SHA256=d594337feca84c26fb67fe87b85e6a5c12fda404b611d905f9d138210c311876
SHELL ["/bin/bash", "-o", "pipefail", "-c"]

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends build-essential \
    && rm -rf /var/lib/apt/lists/*

# The emsdk version Qt recommends for this Qt; see QT_EMCC_RECOMMENDED_VERSION in
# qtbase/cmake/QtPublicWasmToolchainHelpers.cmake.
RUN git clone --depth 1 https://github.com/emscripten-core/emsdk.git /opt/emsdk \
    && /opt/emsdk/emsdk install ${EMSDK_VERSION} \
    && /opt/emsdk/emsdk activate ${EMSDK_VERSION} \
    && rm -rf /opt/emsdk/downloads /opt/emsdk/upstream/emscripten/test

RUN mkdir -p /opt/qt-src && cd /opt/qt-src \
    && base=https://download.qt.io/official_releases/qt/${QT_VERSION%.*}/${QT_VERSION}/submodules \
    && curl -fsSLO ${base}/qtbase-everywhere-src-${QT_VERSION}.tar.xz \
    && curl -fsSLO ${base}/qtsvg-everywhere-src-${QT_VERSION}.tar.xz \
    && echo "${QTBASE_SHA256}  qtbase-everywhere-src-${QT_VERSION}.tar.xz" | sha256sum -c \
    && echo "${QTSVG_SHA256}  qtsvg-everywhere-src-${QT_VERSION}.tar.xz" | sha256sum -c \
    && tar xf qtbase-everywhere-src-${QT_VERSION}.tar.xz \
    && tar xf qtsvg-everywhere-src-${QT_VERSION}.tar.xz \
    && mv qtbase-everywhere-src-${QT_VERSION} qtbase \
    && mv qtsvg-everywhere-src-${QT_VERSION} qtsvg \
    && rm *.tar.xz

# The host Qt whose tools (moc, rcc, uic, ...) run during the cross builds.
RUN mkdir -p /tmp/build/host && cd /tmp/build/host \
    && /opt/qt-src/qtbase/configure -prefix /opt/qt-host -release \
        -nomake examples -nomake tests -no-opengl \
    && cmake --build . --parallel \
    && cmake --install . \
    && rm -rf /tmp/build/host

# Fixes Qt needs to build without style sheet support and without the CBOR
# stream writer; README.md says where each comes from. A fix the sources
# already contain, as a later Qt release will, is skipped.
COPY qt-patches /opt/qt-patches
RUN cd /opt/qt-src/qtbase \
    && for patch in /opt/qt-patches/qtbase/*.patch; do \
           if git apply --reverse --check "$patch" 2> /dev/null; then \
               echo "Already in the sources: $(basename "$patch")"; \
           else \
               git apply "$patch"; \
           fi; \
       done

# Qt for WebAssembly, configured as README.md explains flag by flag. It builds
# the two submodules one at a time instead of through the top-level configure,
# so only their sources need downloading; the -submodules and -skip flags of the
# README's configure line have no counterpart here.
RUN source /opt/emsdk/emsdk_env.sh > /dev/null \
    && mkdir -p /tmp/build/qtbase && cd /tmp/build/qtbase \
    && /opt/qt-src/qtbase/configure -platform wasm-emscripten \
        -prefix /opt/qt-wasm \
        -qt-host-path /opt/qt-host \
        -feature-wasm-exceptions \
        -feature-wasm-jspi \
        -release -optimize-size \
        -nomake examples -nomake tests \
        -no-feature-jpeg -no-feature-gif -no-feature-ico \
        -no-feature-imageformat_jpeg \
        -no-feature-sql -no-feature-testlib \
        -no-feature-mdiarea -no-feature-calendarwidget -no-feature-fontdialog \
        -no-feature-whatsthis -no-feature-sessionmanager \
        -no-feature-printpreviewwidget \
        -no-feature-hijricalendar -no-feature-jalalicalendar \
        -no-feature-islamiccivilcalendar \
        -no-feature-scroller -no-feature-lcdnumber -no-feature-errormessage \
        -no-feature-undogroup -no-feature-socks5 \
        -no-feature-style-stylesheet \
        -no-feature-raster-fp -no-feature-sha3-fast \
        -no-feature-mimetype-database \
        -no-feature-imageformat_bmp -no-feature-imageformat_ppm \
        -no-feature-imageformat_xbm \
        -no-feature-cborstreamwriter -no-feature-datetimeparser \
        -no-feature-sizegrip -no-feature-tabletevent \
        -no-feature-dial -no-feature-toolbox -no-feature-rubberband \
        -no-feature-graphicsview \
    && cmake --build . --parallel \
    && cmake --install . \
    && mkdir -p /tmp/build/qtsvg && cd /tmp/build/qtsvg \
    && /opt/qt-wasm/bin/qt-configure-module /opt/qt-src/qtsvg \
    && cmake --build . --parallel \
    && cmake --install . \
    && rm -rf /tmp/build

FROM runtime
COPY --from=build /opt/emsdk /opt/emsdk
COPY --from=build /opt/qt-host /opt/qt-host
COPY --from=build /opt/qt-wasm /opt/qt-wasm
# qtprofiler links a subset of the fonts Qt bundles for WebAssembly, and finds
# them through the source tree the Qt install records. They are all of the
# sources it needs.
COPY --from=build /opt/qt-src/qtbase/src/3rdparty/wasm /opt/qt-src/qtbase/src/3rdparty/wasm

# What scripts/build-wasm-profiler.py falls back to when not told otherwise.
ENV EMSDK=/opt/emsdk \
    QT_HOST_PATH=/opt/qt-host \
    QT_WASM_ROOT=/opt/qt-wasm

# emcc builds the system libraries a link needs into its cache on first use, so
# the cache has to be writable for whichever user the container runs as.
RUN chmod -R a+rwX /opt/emsdk/upstream/emscripten/cache

WORKDIR /src
ENTRYPOINT ["python3", "/src/scripts/build-wasm-profiler.py", \
            "--build", "/src/builds/wasm-docker", \
            "--output", "/src/builds/wasm-docker/dist", \
            "--clean"]
