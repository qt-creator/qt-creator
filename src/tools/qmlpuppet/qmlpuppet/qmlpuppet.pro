TARGET = qmlpuppet
QT += script-private

TEMPLATE = app

include(../../../../qtcreator.pri)
DESTDIR = $$IDE_BIN_PATH
include(../../../rpath.pri)

include(../../../../share/qtcreator/qml/qmlpuppet/qmlpuppet/qmlpuppet.pri)

isEmpty(PRECOMPILED_HEADER):PRECOMPILED_HEADER = $$PWD/../../../shared/qtcreator_pch.h
