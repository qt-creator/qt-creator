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
        diffeditorreloader.h \
        differ.h \
        diffutils.h \
        selectabletexteditorwidget.h \
        sidebysidediffeditorwidget.h \
        unifieddiffeditorwidget.h

SOURCES += diffeditor.cpp \
        diffeditorcontroller.cpp \
        diffeditordocument.cpp \
        diffeditorfactory.cpp \
        diffeditorguicontroller.cpp \
        diffeditormanager.cpp \
        diffeditorplugin.cpp \
        diffeditorreloader.cpp \
        differ.cpp \
        diffutils.cpp \
        selectabletexteditorwidget.cpp \
        sidebysidediffeditorwidget.cpp \
        unifieddiffeditorwidget.cpp

RESOURCES += diffeditor.qrc
