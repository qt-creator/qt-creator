QT       = core network

include (../../../qtcreator.pri)
include (../../../src/libs/ssh/ssh.pri)

macx:QMAKE_LFLAGS += -Wl,-rpath,\"$$IDE_BIN_PATH/..\"
INCLUDEPATH *= $$IDE_SOURCE_TREE/src/plugins
LIBS *= -L$$IDE_LIBRARY_PATH
unix {
    QMAKE_LFLAGS += -Wl,-rpath,\"$$IDE_LIBRARY_PATH\"
}

CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE = app

DEPENDPATH+=.
INCLUDEPATH+=.
