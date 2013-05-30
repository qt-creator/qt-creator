DEFINES += DIFFEDITOR_LIBRARY
include(../../qtcreatorplugin.pri)

HEADERS += diffeditor_global.h \
        diffeditorconstants.h \
        diffeditor.h \
        diffeditorfile.h \
        diffeditorplugin.h \
        diffeditorwidget.h \
        differ.h

SOURCES += diffeditor.cpp \
        diffeditorfile.cpp \
        diffeditorplugin.cpp \
        diffeditorwidget.cpp \
        differ.cpp

RESOURCES +=
