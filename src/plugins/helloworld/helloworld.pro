TEMPLATE = lib
TARGET = HelloWorld

include(../../qworkbenchplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)

HEADERS += helloworldplugin.h \
    helloworldwindow.h

SOURCES += helloworldplugin.cpp \
    helloworldwindow.cpp
    
