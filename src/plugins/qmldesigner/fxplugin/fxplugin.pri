TARGET = fxplugin
TEMPLATE = lib
CONFIG += plugin

QT += script \
      declarative

include (../core/iwidgetplugin.pri)

DEFINES += FX_LIBRARY
SOURCES += $$PWD/fxplugin.cpp

HEADERS += $$PWD/fxplugin.h  $$PWD/../core/include/iwidgetplugin.h

RESOURCES += $$PWD/fxplugin.qrc

OTHER_FILES += $$PWD/fx.metainfo
