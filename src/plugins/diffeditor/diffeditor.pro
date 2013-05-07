DEFINES += DIFFEDITOR_LIBRARY
include(../../qtcreatorplugin.pri)

HEADERS += diffeditor_global.h \
        diffeditorconstants.h \
        diffeditoreditable.h \
        diffeditorfile.h \
        diffeditorplugin.h \
        diffeditorwidget.h \
        differ.h

SOURCES += diffeditoreditable.cpp \
        diffeditorfile.cpp \
        diffeditorplugin.cpp \
        diffeditorwidget.cpp \
        differ.cpp

RESOURCES +=
