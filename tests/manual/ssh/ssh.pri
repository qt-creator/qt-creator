include (../../../qtcreator.pri)
include (../../../src/plugins/coreplugin/coreplugin.pri)

LIBS += -L$$IDE_PLUGIN_PATH/Nokia

macx:QMAKE_LFLAGS += -Wl,-rpath,\"$$IDE_BIN_PATH/..\"
INCLUDEPATH *= $$IDE_SOURCE_TREE/src/plugins
LIBS *= -L$$IDE_LIBRARY_PATH/Nokia

QT       += core
QT       -= gui
CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE = app

DEPENDPATH+=.
INCLUDEPATH+=.

unix {
     copy_script.commands = cp -a $${PWD}/call.sh $$OUT_PWD/$${TARGET}.sh
     first.depends = $(first) copy_script
     QMAKE_EXTRA_TARGETS += first copy_script
}
