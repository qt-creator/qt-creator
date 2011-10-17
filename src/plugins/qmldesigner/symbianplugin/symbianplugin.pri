TARGET = symbianplugin
TEMPLATE = lib
CONFIG += plugin

QT += script \
      declarative

include (../designercore/iwidgetplugin.pri)

DEFINES += SYMBIAN_LIBRARY
SOURCES += $$PWD/symbianplugin.cpp

HEADERS += $$PWD/symbianplugin.h  $$PWD/../designercore/include/iwidgetplugin.h

RESOURCES += $$PWD/symbianplugin.qrc

OTHER_FILES += $$PWD/symbian.metainfo
