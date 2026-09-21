# Qt builds its Linux binaries for arm64 on a much newer base than the ones
# for x86_64: Qt 6.8.3 for arm64 needs glibc 2.38 and GLIBCXX_3.4.32, so its
# moc does not run on Ubuntu 22.04, which the amd64 container uses for GCC 11.
# The oldest GCC of this distribution is 12; the amd64 container is the one
# that holds the documented minimum of GCC 11.
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y \
    g++-12 gdb \
    libgl1-mesa-dev \
    clang-format clang-tidy clang-tools clang clangd libc++-dev libc++1 libc++abi-dev libc++abi1 libclang-dev libclang1 liblldb-dev libllvm-ocaml-dev libomp-dev libomp5 lld lldb llvm-dev llvm-runtime llvm python3-clang python3-lldb \
    cmake \
    curl ca-certificates \
    nano git \
    ninja-build \
    libglib2.0-0t64 \
    python3-venv \
    libxcb-xkb-dev libxkbcommon-dev libxkbcommon-x11-dev \
    libdbus-c++-dev

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 30 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 30

# The CmdBridge server is written in Go; its go.mod requires 1.21.7,
# which is newer than the golang-go of this distribution.
ARG GO_VERSION=1.22.12
RUN curl -fsSL https://go.dev/dl/go${GO_VERSION}.linux-arm64.tar.gz | tar -C /usr/local -xz
ENV PATH="/usr/local/go/bin:$PATH"

RUN python3 -m venv /opt/aqt && /opt/aqt/bin/pip install aqtinstall

# Keep in sync with IDE_QT_VERSION_MIN in cmake/QtCreatorAPI.cmake.
RUN /opt/aqt/bin/aqt install-qt linux_arm64 desktop 6.8.3 linux_gcc_arm64 --modules qt5compat
ENV PATH="/6.8.3/gcc_arm64/bin:$PATH"
