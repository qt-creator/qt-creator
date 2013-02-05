TARGET = componentsplugin
TEMPLATE = lib
CONFIG += plugin

include (../designercore/iwidgetplugin.pri)

DEFINES += COMPONENTS_LIBRARY
SOURCES += $$PWD/componentsplugin.cpp

HEADERS += $$PWD/componentsplugin.h  $$PWD/../designercore/include/iwidgetplugin.h

RESOURCES += $$PWD/componentsplugin.qrc

OTHER_FILES += $$PWD/components.metainfo
