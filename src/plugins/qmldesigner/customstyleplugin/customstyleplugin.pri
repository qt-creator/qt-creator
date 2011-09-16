TARGET = customstyleplugin
TEMPLATE = lib
CONFIG += plugin

QT += script \
      declarative

include (../designercore/iwidgetplugin.pri)

DEFINES += SYMBIAN_LIBRARY
SOURCES += $$PWD/customstyleplugin.cpp

HEADERS += $$PWD/customstyleplugin.h  $$PWD/../designercore/include/iwidgetplugin.h

RESOURCES += $$PWD/customstyleplugin.qrc

OTHER_FILES += $$PWD/customstyle.metainfo

!macx {
    target.path  = /$$IDE_LIBRARY_BASENAME/qmldesigner
    INSTALLS    += target
}
