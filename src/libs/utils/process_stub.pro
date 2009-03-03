TEMPLATE        = app
TARGET          = qtcreator_process_stub
DESTDIR         = ../../../bin

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

