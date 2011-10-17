TARGET = desktopplugin
TEMPLATE = lib
CONFIG += plugin

QT += script \
      declarative

include (../designercore/iwidgetplugin.pri)

DEFINES += SYMBIAN_LIBRARY
SOURCES += $$PWD/desktopplugin.cpp

HEADERS += $$PWD/desktopplugin.h  $$PWD/../designercore/include/iwidgetplugin.h

RESOURCES += $$PWD/desktopplugin.qrc

OTHER_FILES += $$PWD/desktop.metainfo
