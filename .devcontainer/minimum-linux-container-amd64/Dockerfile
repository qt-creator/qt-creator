FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y \
    g++-11 gdb \
    libgl1-mesa-dev \
    clang-format clang-tidy clang-tools clang clangd libc++-dev libc++1 libc++abi-dev libc++abi1 libclang-dev libclang1 liblldb-dev libllvm-ocaml-dev libomp-dev libomp5 lld lldb llvm-dev llvm-runtime llvm python3-clang python3-lldb \
    cmake \
    curl ca-certificates \
    nano git \
    ninja-build \
    libglib2.0-0 \
    pip \
    libxcb-xkb-dev libxkbcommon-dev libxkbcommon-x11-dev \
    libdbus-c++-dev

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 30 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 30

# The CmdBridge server is written in Go; its go.mod requires 1.21.7,
# which is newer than the golang-go of this distribution.
ARG GO_VERSION=1.22.12
RUN curl -fsSL https://go.dev/dl/go${GO_VERSION}.linux-amd64.tar.gz | tar -C /usr/local -xz
ENV PATH="/usr/local/go/bin:$PATH"

RUN pip install aqtinstall

# Keep in sync with IDE_QT_VERSION_MIN in cmake/QtCreatorAPI.cmake.
RUN aqt install-qt linux desktop 6.8.3 linux_gcc_64 --modules qt5compat
ENV PATH="/6.8.3/gcc_64/bin:$PATH"
