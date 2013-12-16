DEFINES += DIFFEDITOR_LIBRARY
include(../../qtcreatorplugin.pri)

HEADERS += diffeditor_global.h \
        diffeditor.h \
        diffeditorconstants.h \
        diffeditorcontroller.h \
        diffeditorfactory.h \
        diffeditorfile.h \
        diffeditorplugin.h \
        diffeditorwidget.h \
        differ.h \
        diffshoweditor.h \
        diffshoweditorfactory.h

SOURCES += diffeditor.cpp \
        diffeditorcontroller.cpp \
        diffeditorfactory.cpp \
        diffeditorfile.cpp \
        diffeditorplugin.cpp \
        diffeditorwidget.cpp \
        differ.cpp \
        diffshoweditor.cpp \
        diffshoweditorfactory.cpp

RESOURCES +=
