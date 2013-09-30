TARGET = qml2puppet

TEMPLATE = app

include(../../../../qtcreator.pri)

DESTDIR = $$IDE_BIN_PATH
include(../../../rpath.pri)

include(../../../../share/qtcreator/qml/qmlpuppet/qml2puppet/qml2puppet.pri)

isEmpty(PRECOMPILED_HEADER):PRECOMPILED_HEADER = $$PWD/../../../shared/qtcreator_pch.h
