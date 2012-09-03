!win32: error("process_ctrlc_stub is Windows only")

CONFIG    -= qt
CONFIG    += console warn_on

include(../../../qtcreator.pri)

TEMPLATE  = app
TARGET    = qtcreator_ctrlc_stub
DESTDIR   = $$IDE_LIBEXEC_PATH

SOURCES   += process_ctrlc_stub.cpp
LIBS      += -luser32 -lshell32

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

target.path  = $$QTC_PREFIX/bin # FIXME: libexec, more or less
INSTALLS    += target
