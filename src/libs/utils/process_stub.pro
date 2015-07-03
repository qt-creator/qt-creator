CONFIG          += warn_on console use_c_linker
CONFIG          -= qt app_bundle

include(../../../qtcreator.pri)

TEMPLATE        = app
TARGET          = qtcreator_process_stub
DESTDIR         = $$IDE_LIBEXEC_PATH

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

unix {
    SOURCES += process_stub_unix.c
    solaris-.*: LIBS += -lsocket
} else {
    SOURCES += process_stub_win.c
    LIBS += -lshell32
}

target.path  = $$QTC_PREFIX/$$relative_path($$IDE_LIBEXEC_PATH, $$IDE_BUILD_TREE)
INSTALLS    += target
