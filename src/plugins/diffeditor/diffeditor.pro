DEFINES += DIFFEDITOR_LIBRARY
include(../../qtcreatorplugin.pri)

HEADERS += diffeditor_global.h \
        diffeditor.h \
        diffeditorconstants.h \
        diffeditorcontroller.h \
        diffeditordocument.h \
        diffeditorfactory.h \
        diffeditorplugin.h \
        diffeditorwidgetcontroller.h \
        differ.h \
        diffutils.h \
        diffview.h \
        selectabletexteditorwidget.h \
        sidebysidediffeditorwidget.h \
        unifieddiffeditorwidget.h \
        diffeditoricons.h

SOURCES += diffeditor.cpp \
        diffeditorcontroller.cpp \
        diffeditordocument.cpp \
        diffeditorfactory.cpp \
        diffeditorplugin.cpp \
        diffeditorwidgetcontroller.cpp \
        differ.cpp \
        diffutils.cpp \
        diffview.cpp \
        selectabletexteditorwidget.cpp \
        sidebysidediffeditorwidget.cpp \
        unifieddiffeditorwidget.cpp

RESOURCES += diffeditor.qrc
