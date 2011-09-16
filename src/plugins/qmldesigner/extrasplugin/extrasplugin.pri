TARGET = extrasplugin
TEMPLATE = lib
CONFIG += plugin

QT += script \
      declarative

include (../designercore/iwidgetplugin.pri)

DEFINES += SYMBIAN_LIBRARY
SOURCES += $$PWD/extrasplugin.cpp

HEADERS += $$PWD/extrasplugin.h  $$PWD/../designercore/include/iwidgetplugin.h

RESOURCES += $$PWD/extrasplugin.qrc

OTHER_FILES += $$PWD/extras.metainfo

!macx {
    target.path  = /$$IDE_LIBRARY_BASENAME/qmldesigner
    INSTALLS    += target
}
