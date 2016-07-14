include(../../qtcreatorplugin.pri)

DEFINES += BINEDITOR_LIBRARY

HEADERS += bineditorplugin.h \
        bineditorservice.h \
        bineditorwidget.h \
        bineditorconstants.h \
        bineditor_global.h \
        markup.h

SOURCES += bineditorplugin.cpp \
        bineditorwidget.cpp \
        markup.cpp
