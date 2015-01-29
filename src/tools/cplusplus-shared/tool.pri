QTC_LIB_DEPENDS += cplusplus

include(../../../qtcreator.pri)
include(../../rpath.pri)

DESTDIR = $$IDE_BIN_PATH

QT = core gui
TEMPLATE = app

osx:CONFIG -= app_bundle
CONFIG += console
