QT += network declarative

include(../../qtcreatorplugin.pri)

HEADERS += welcomeplugin.h \
    welcome_global.h

SOURCES += welcomeplugin.cpp

DEFINES += WELCOME_LIBRARY

QML_IMPORT_PATH = $$IDE_SOURCE_TREE/lib/qtcreator/
