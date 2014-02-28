DEFINES += DIFFEDITOR_LIBRARY
include(../../qtcreatorplugin.pri)

HEADERS += diffeditor_global.h \
        diffeditor.h \
        diffeditorconstants.h \
        diffeditorcontroller.h \
        diffeditordocument.h \
        diffeditorfactory.h \
        diffeditorguicontroller.h \
        diffeditormanager.h \
        diffeditorplugin.h \
        differ.h \
        diffutils.h \
        sidebysidediffeditorwidget.h

SOURCES += diffeditor.cpp \
        diffeditorcontroller.cpp \
        diffeditordocument.cpp \
        diffeditorfactory.cpp \
        diffeditorguicontroller.cpp \
        diffeditormanager.cpp \
        diffeditorplugin.cpp \
        differ.cpp \
        diffutils.cpp \
        sidebysidediffeditorwidget.cpp

RESOURCES +=
