TEMPLATE = lib
CONFIG += shared
linux-* {
CONFIG -= release
CONFIG += debug
}
SOURCES=gdbmacros.cpp
