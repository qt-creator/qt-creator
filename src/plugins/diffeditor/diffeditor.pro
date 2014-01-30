DEFINES += DIFFEDITOR_LIBRARY
include(../../qtcreatorplugin.pri)

HEADERS += diffeditor_global.h \
        diffeditor.h \
        diffeditorconstants.h \
        diffeditorcontroller.h \
        diffeditordocument.h \
        diffeditorfactory.h \
        diffeditorplugin.h \
        diffeditorwidget.h \
        differ.h \
        diffshoweditor.h \
        diffshoweditorfactory.h

SOURCES += diffeditor.cpp \
        diffeditorcontroller.cpp \
        diffeditordocument.cpp \
        diffeditorfactory.cpp \
        diffeditorplugin.cpp \
        diffeditorwidget.cpp \
        differ.cpp \
        diffshoweditor.cpp \
        diffshoweditorfactory.cpp

RESOURCES +=
