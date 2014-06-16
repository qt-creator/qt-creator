
QT += quick

!isEmpty(USE_QUICK_WIDGET) {
    QT +=  quickwidgets
    DEFINES += USE_QUICK_WIDGET
}

include(../../qtcreatorplugin.pri)

HEADERS += welcomeplugin.h

SOURCES += welcomeplugin.cpp

DEFINES += WELCOME_LIBRARY
