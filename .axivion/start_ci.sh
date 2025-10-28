#!/usr/bin/bash

if [ -z "$QT_DIR" ]; then
    echo "You need to set QT_DIR environment variable to proceed."
    exit 1
fi
if [ -z "$LLVM_INSTALL_DIR" ]; then
    echo "You need to set LLVM_INSTALL_DIR environment variable to proceed."
    exit 1
fi
if [ -z "$(which cmake)" ]; then
    echo "cmake needs to be in PATH"
    exit 1
fi
if [ -z "$(which ninja)" ]; then
    echo "ninja needs to be in PATH"
    exit 1
fi
if [ -z "$(which gcc)" ] || [ -z "$(which g++)" ]; then
    echo "gcc/g++ needs to be in PATH"
    exit 1
fi

. $HOME/bauhaus-suite/bauhaus-kshrc

BAUHAUS_CONFIG=$(cd $(dirname $(readlink -f $0)) && pwd)
export BAUHAUS_CONFIG

# generate compiler config
gccsetup --cc gcc --cxx g++ --config "$BAUHAUS_CONFIG/compiler_config.json"

# -j1 limits workers - e.g. ir2rfg single worker uses ~23GB
axivion_ci -j1

