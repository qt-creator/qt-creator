TEMPLATE = lib
TARGET = Welcome
QT += network
include(../../qtcreatorplugin.pri)
include(welcome_dependencies.pri)
HEADERS += welcomeplugin.h \
    welcomemode.h \
    communitywelcomepagewidget.h \
    communitywelcomepage.h \
    welcome_global.h
SOURCES += welcomeplugin.cpp \
    welcomemode.cpp \
    communitywelcomepagewidget.cpp \
    communitywelcomepage.cpp

FORMS += welcomemode.ui \
    communitywelcomepagewidget.ui
RESOURCES += welcome.qrc
DEFINES += WELCOME_LIBRARY
OTHER_FILES += Welcome.pluginspec
