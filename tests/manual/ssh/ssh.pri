QT       = core network

QTC_LIB_DEPENDS += ssh utils
include (../../../qtcreator.pri)

macx:QMAKE_LFLAGS += -Wl,-rpath,\"$$IDE_BIN_PATH/..\"
LIBS *= -L$$IDE_LIBRARY_PATH
unix {
    QMAKE_LFLAGS += -Wl,-rpath,\"$$IDE_LIBRARY_PATH\"
}

CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE = app

DEPENDPATH+=.
INCLUDEPATH+=.
