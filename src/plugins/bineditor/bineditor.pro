TEMPLATE = lib
TARGET = BinEditor
include(../../qtcreatorplugin.pri)
include(bineditor_dependencies.pri)

DEFINES += QT_NO_CAST_FROM_ASCII

HEADERS += bineditorplugin.h \
        bineditor.h \
        bineditorconstants.h \
        markup.h

SOURCES += bineditorplugin.cpp \
        bineditor.cpp
