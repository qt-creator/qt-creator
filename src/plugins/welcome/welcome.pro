
QT += quick quickwidgets

QML_IMPORT_PATH=../../../share/qtcreator/welcomescreen

include(../../qtcreatorplugin.pri)

HEADERS += welcomeplugin.h

SOURCES += welcomeplugin.cpp

DEFINES += WELCOME_LIBRARY

RESOURCES += \
    welcome.qrc
