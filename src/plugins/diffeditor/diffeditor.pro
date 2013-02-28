TEMPLATE = lib
TARGET = DiffEditor
DEFINES += DIFFEDITOR_LIBRARY
include(../../qtcreatorplugin.pri)
include(diffeditor_dependencies.pri)

HEADERS += diffeditorplugin.h \
        diffeditorwidget.h \
        diffeditorconstants.h \
        diffeditor_global.h \
        differ.h

SOURCES += diffeditorplugin.cpp \
        diffeditorwidget.cpp \
        differ.cpp

RESOURCES +=
