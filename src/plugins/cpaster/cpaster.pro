QT += network
TEMPLATE = lib
TARGET = CodePaster

include(../../qworkbenchplugin.pri)
include(cpaster_dependencies.pri)

HEADERS += cpasterplugin.h \
    settingspage.h
SOURCES += cpasterplugin.cpp \
    settingspage.cpp
FORMS += settingspage.ui \
    pasteselect.ui

include(../../shared/cpaster/cpaster.pri)
