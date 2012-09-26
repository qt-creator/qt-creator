TARGET = extrasplugin
TEMPLATE = lib
CONFIG += plugin

include (../designercore/iwidgetplugin.pri)

SOURCES += $$PWD/extrasplugin.cpp

HEADERS += $$PWD/extrasplugin.h  $$PWD/../designercore/include/iwidgetplugin.h

RESOURCES += $$PWD/extrasplugin.qrc

OTHER_FILES += $$PWD/extras.metainfo
