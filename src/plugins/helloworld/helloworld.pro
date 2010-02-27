TEMPLATE = lib
TARGET = HelloWorld

include(../../qtcreatorplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)

HEADERS += helloworldplugin.h \
    helloworldwindow.h

SOURCES += helloworldplugin.cpp \
    helloworldwindow.cpp

OTHER_FILES += helloworld.pluginspec
