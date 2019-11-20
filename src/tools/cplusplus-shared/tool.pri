QTC_LIB_DEPENDS += cplusplus utils

include(../../../qtcreator.pri)

RPATH_BASE = $$IDE_BIN_PATH
include(../../rpath.pri)

DESTDIR = $$IDE_BIN_PATH

QT = core gui
TEMPLATE = app

osx:CONFIG -= app_bundle
CONFIG += console
