TEMPLATE = lib
TARGET = FakeVim

# CONFIG += single
include(../../qworkbenchplugin.pri)
include(../../plugins/projectexplorer/projectexplorer.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/texteditor/texteditor.pri)

# DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII
QT += gui

SOURCES += \
    handler.cpp \
    fakevimplugin.cpp

HEADERS += \
    handler.h \
    fakevimplugin.h
