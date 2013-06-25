DEFINES += DIFFEDITOR_LIBRARY
include(../../qtcreatorplugin.pri)

HEADERS += diffeditor_global.h \
        diffeditorconstants.h \
        diffeditor.h \
        diffeditorfactory.h \
        diffeditorfile.h \
        diffeditorplugin.h \
        diffeditorwidget.h \
        differ.h \
        diffshoweditor.h \
        diffshoweditorfactory.h

SOURCES += diffeditor.cpp \
        diffeditorfactory.cpp \
        diffeditorfile.cpp \
        diffeditorplugin.cpp \
        diffeditorwidget.cpp \
        differ.cpp \
        diffshoweditor.cpp \
        diffshoweditorfactory.cpp

RESOURCES +=
