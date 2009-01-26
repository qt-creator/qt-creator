TEMPLATE = lib
TARGET = FakeVim

# CONFIG += single
include(../../libs/cplusplus/cplusplus.pri)
include(../../qworkbenchplugin.pri)
include(../../plugins/projectexplorer/projectexplorer.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/texteditor/texteditor.pri)
include(../../plugins/texteditor/cppeditor.pri)
include(../../shared/indenter/indenter.pri)

# DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII
QT += gui

SOURCES += \
    fakevimhandler.cpp \
    fakevimplugin.cpp

HEADERS += \
    fakevimconstants.h \
    fakevimhandler.h \
    fakevimplugin.h
