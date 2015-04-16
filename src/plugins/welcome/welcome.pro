
QT += quick

QML_IMPORT_PATH=../../../share/qtcreator/welcomescreen

include(../../qtcreatorplugin.pri)

!isEmpty(USE_QUICK_WIDGET)|minQtVersion(5, 5, 0) {
    QT +=  quickwidgets
    DEFINES += USE_QUICK_WIDGET
}

HEADERS += welcomeplugin.h

SOURCES += welcomeplugin.cpp

DEFINES += WELCOME_LIBRARY

RESOURCES += \
    welcome.qrc
