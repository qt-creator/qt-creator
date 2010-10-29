include (../../../qtcreator.pri)
include (../../../src/plugins/coreplugin/coreplugin.pri)

INCLUDEPATH *= $$IDE_SOURCE_TREE/src/plugins
LIBS *= -L$$IDE_LIBRARY_PATH/Nokia

QT       += core
QT       -= gui
CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE = app
DEPENDPATH+=.
INCLUDEPATH+=.
