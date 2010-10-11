INCLUDEPATH += ../../../../src/plugins
CREATORLIBPATH = ../../../../lib/qtcreator
PLUGINPATH=$$CREATORLIBPATH/plugins/Nokia
LIBS *= -L$$PLUGINPATH -lCore
LIBS *= -L$$CREATORLIBPATH
include (../../../qtcreator.pri)
include (../../../src/plugins/coreplugin/coreplugin_dependencies.pri)
QT       += core
QT       -= gui
CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE = app
DEPENDPATH+=.
INCLUDEPATH+=.