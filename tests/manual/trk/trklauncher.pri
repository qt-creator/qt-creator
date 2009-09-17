DEFINES += DEBUG_TRK=0
TRK_DIR = ../../../src/plugins/debugger/gdb

INCLUDEPATH *= $$PWD $$TRK_DIR

SOURCES += \
    $$TRK_DIR/trkutils.cpp \
    $$PWD/trkdevice.cpp \
    $$PWD/launcher.cpp \

HEADERS += \
    $$TRK_DIR/trkutils.h \
    $$TRK_DIR/callback.h \
    $$PWD/trkdevice.h \
    $$PWD/launcher.h
