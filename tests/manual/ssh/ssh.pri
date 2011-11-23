QT       = core

include (../../../qtcreator.pri)
include (../../../src/plugins/coreplugin/coreplugin.pri)

LIBS += -L$$IDE_PLUGIN_PATH/Nokia

macx:QMAKE_LFLAGS += -Wl,-rpath,\"$$IDE_BIN_PATH/..\"
INCLUDEPATH *= $$IDE_SOURCE_TREE/src/plugins
LIBS *= -L$$IDE_LIBRARY_PATH/Nokia
unix {
    QMAKE_LFLAGS += -Wl,-rpath,\"$$IDE_PLUGIN_PATH/..\"
    QMAKE_LFLAGS += -Wl,-rpath,\"$$IDE_PLUGIN_PATH/Nokia\"
}

CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE = app

DEPENDPATH+=.
INCLUDEPATH+=.
