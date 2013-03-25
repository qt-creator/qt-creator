TEMPLATE = lib
TARGET = HelloWorld

include(../../qtcreatorplugin.pri)

HEADERS += helloworldplugin.h \
    helloworldwindow.h

SOURCES += helloworldplugin.cpp \
    helloworldwindow.cpp
