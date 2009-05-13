TEMPLATE = lib
TARGET = RegExp

include(../../qtcreatorplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)

QT += xml

HEADERS += regexpwindow.h regexpplugin.h settings.h
SOURCES += regexpwindow.cpp regexpplugin.cpp settings.cpp
