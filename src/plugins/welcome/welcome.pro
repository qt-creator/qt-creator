TEMPLATE = lib
TARGET = Welcome
QT += network
include(../../qtcreatorplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)

HEADERS += welcomeplugin.h \
           welcomemode.h \
           welcomemode_p.h \
           rssfetcher.h

SOURCES += welcomeplugin.cpp \
           welcomemode.cpp \
           rssfetcher.cpp

FORMS += welcomemode.ui
    
RESOURCES += welcome.qrc

DEFINES += WELCOME_LIBRARY

OTHER_FILES += Welcome.pluginspec
