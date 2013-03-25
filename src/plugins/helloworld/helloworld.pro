TEMPLATE = lib
TARGET = HelloWorld

include(../../qtcreatorplugin.pri)
include(helloworld_dependencies.pri)

HEADERS += helloworldplugin.h \
    helloworldwindow.h

SOURCES += helloworldplugin.cpp \
    helloworldwindow.cpp
