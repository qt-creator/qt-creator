TARGET = qtquickplugin
TEMPLATE = lib
CONFIG += plugin

include (../designercore/iwidgetplugin.pri)

DEFINES += QTQUICK_LIBRARY
SOURCES += $$PWD/qtquickplugin.cpp

HEADERS += $$PWD/qtquickplugin.h  $$PWD/../designercore/include/iwidgetplugin.h

RESOURCES += $$PWD/qtquickplugin.qrc

OTHER_FILES += $$PWD/quick.metainfo
