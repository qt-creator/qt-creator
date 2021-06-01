
FROM ubuntu:20.04

RUN apt-get update \
 && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    git \
    openssh-client \
    sudo \
    vim \
    wget \
    cmake \
    qtbase5-dev \
    libqt5core5a \
    libqt5widgets5 \
    libqt5quick5 \
    libqt5quickcontrols2-5 \
    qt5-qmake \
    g++ \
    gdb \
    ninja-build \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*
