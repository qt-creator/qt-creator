TEMPLATE = lib
TARGET = Welcome
QT += network declarative

include(../../qtcreatorplugin.pri)
include(welcome_dependencies.pri)

HEADERS += welcomeplugin.h \
    welcome_global.h

SOURCES += welcomeplugin.cpp

RESOURCES += welcome.qrc

DEFINES += WELCOME_LIBRARY
