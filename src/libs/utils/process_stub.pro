IDE_BUILD_TREE=../../../
include(../../qworkbench.pri)

TEMPLATE        = app
TARGET          = qtcreator_process_stub
macx {
    DESTDIR         = $$IDE_BUILD_TREE/bin/$${IDE_APP_TARGET}.app/Contents/Resources
} else {
    DESTDIR         = ../../../bin
}

CONFIG          += warn_on console use_c_linker
CONFIG          -= qt app_bundle

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

unix {
    SOURCES += process_stub_unix.c
} else {
    SOURCES += process_stub_win.c
    LIBS += -lshell32
}

