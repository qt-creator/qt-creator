TARGET = meegoplugin
TEMPLATE = lib
CONFIG += plugin

QT += script \
      declarative

include (../designercore/iwidgetplugin.pri)

DEFINES += SYMBIAN_LIBRARY
SOURCES += $$PWD/meegoplugin.cpp

HEADERS += $$PWD/meegoplugin.h  $$PWD/../designercore/include/iwidgetplugin.h

RESOURCES += $$PWD/meegoplugin.qrc

OTHER_FILES += $$PWD/meego.metainfo

!macx {
    target.path  = /$$IDE_LIBRARY_BASENAME/qmldesigner
    INSTALLS    += target
}
