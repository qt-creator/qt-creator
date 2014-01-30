DEFINES += DIFFEDITOR_LIBRARY
include(../../qtcreatorplugin.pri)

HEADERS += diffeditor_global.h \
        diffeditor.h \
        diffeditorconstants.h \
        diffeditorcontroller.h \
        diffeditordocument.h \
        diffeditorfactory.h \
        diffeditorplugin.h \
        differ.h \
        diffshoweditor.h \
        diffshoweditorfactory.h \
        sidebysidediffeditorwidget.h

SOURCES += diffeditor.cpp \
        diffeditorcontroller.cpp \
        diffeditordocument.cpp \
        diffeditorfactory.cpp \
        diffeditorplugin.cpp \
        differ.cpp \
        diffshoweditor.cpp \
        diffshoweditorfactory.cpp \
        sidebysidediffeditorwidget.cpp

RESOURCES +=
