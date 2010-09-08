TEMPLATE = lib
TARGET = BinEditor
include(../../qtcreatorplugin.pri)
include(bineditor_dependencies.pri)

HEADERS += bineditorplugin.h \
        bineditor.h \
        bineditorconstants.h

SOURCES += bineditorplugin.cpp \
        bineditor.cpp

RESOURCES +=

OTHER_FILES += BinEditor.pluginspec
