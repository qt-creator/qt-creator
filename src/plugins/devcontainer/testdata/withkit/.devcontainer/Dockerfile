FROM ubuntu:25.10

RUN apt-get update && \
    apt-get install -y \
    libgl1-mesa-dev \
    clang-format clang-tidy clang-tools clang clangd libc++-dev libc++1 libc++abi-dev libc++abi1 libclang-dev libclang1 liblldb-dev libllvm-ocaml-dev libomp-dev libomp5 lld lldb llvm-dev llvm-runtime llvm python3-clang python3-lldb \
    cmake \
    nano \
    ninja-build \
    libglib2.0-0 \
    pip \
    libxcb-xkb-dev libxkbcommon-dev libxkbcommon-x11-dev \
    libdbus-c++-dev \
    python3-venv

RUN python3 -m venv /tmp/.venv && \
    . /tmp/.venv/bin/activate && \
    pip install aqtinstall && \
    aqt install-qt linux desktop 6.7.0 linux_gcc_64 --modules qt5compat && \
    rm -rf /tmp/.venv


ENV PATH="/6.7.0/gcc_64/bin:$PATH"
