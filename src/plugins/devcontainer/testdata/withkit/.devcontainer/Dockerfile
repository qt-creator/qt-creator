FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y \
    g++-10 \
    libgl1-mesa-dev \
    clang-format clang-tidy clang-tools clang clangd libc++-dev libc++1 libc++abi-dev libc++abi1 libclang-dev libclang1 liblldb-dev libllvm-ocaml-dev libomp-dev libomp5 lld lldb llvm-dev llvm-runtime llvm python3-clang python3-lldb \
    cmake \
    nano \
    ninja-build \
    libglib2.0-0 \
    pip \
    libxcb-xkb-dev libxkbcommon-dev libxkbcommon-x11-dev \
    libdbus-c++-dev

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 30 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 30

RUN pip install aqtinstall

RUN aqt install-qt linux desktop 6.7.0 linux_gcc_64 --modules qt5compat
ENV PATH="/6.7.0/gcc_64/bin:$PATH"
