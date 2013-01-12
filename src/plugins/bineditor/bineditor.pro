TEMPLATE = lib
TARGET = BinEditor
include(../../qtcreatorplugin.pri)
include(bineditor_dependencies.pri)

HEADERS += bineditorplugin.h \
        bineditor.h \
        bineditorconstants.h \
        markup.h

SOURCES += bineditorplugin.cpp \
        bineditor.cpp
