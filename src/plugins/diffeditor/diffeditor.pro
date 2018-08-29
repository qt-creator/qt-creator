DEFINES += DIFFEDITOR_LIBRARY
include(../../qtcreatorplugin.pri)

HEADERS += \
        descriptionwidgetwatcher.h \
        diffeditor_global.h \
        diffeditor.h \
        diffeditorconstants.h \
        diffeditorcontroller.h \
        diffeditordocument.h \
        diffeditorfactory.h \
        diffeditorplugin.h \
        diffeditorwidgetcontroller.h \
        diffutils.h \
        diffview.h \
        selectabletexteditorwidget.h \
        sidebysidediffeditorwidget.h \
        unifieddiffeditorwidget.h \
        diffeditoricons.h

SOURCES += \
        descriptionwidgetwatcher.cpp \
        diffeditor.cpp \
        diffeditorcontroller.cpp \
        diffeditordocument.cpp \
        diffeditorfactory.cpp \
        diffeditorplugin.cpp \
        diffeditorwidgetcontroller.cpp \
        diffutils.cpp \
        diffview.cpp \
        selectabletexteditorwidget.cpp \
        sidebysidediffeditorwidget.cpp \
        unifieddiffeditorwidget.cpp

RESOURCES += diffeditor.qrc
