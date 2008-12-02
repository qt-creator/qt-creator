TEMPLATE = lib
TARGET = RegExp

include(../../qworkbenchplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)

QT += xml

HEADERS += regexpwindow.h regexpplugin.h settings.h
SOURCES += regexpwindow.cpp regexpplugin.cpp settings.cpp
