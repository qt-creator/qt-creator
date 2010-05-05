TARGET = fxplugin
TEMPLATE = lib
CONFIG += plugin

QT += script \
      declarative

include (../designercore/iwidgetplugin.pri)

DEFINES += FX_LIBRARY
SOURCES += $$PWD/fxplugin.cpp

HEADERS += $$PWD/fxplugin.h  $$PWD/../designercore/include/iwidgetplugin.h

RESOURCES += $$PWD/fxplugin.qrc

OTHER_FILES += $$PWD/fx.metainfo
