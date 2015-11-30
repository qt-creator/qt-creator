TARGET = qml2puppet

TEMPLATE = app

include(../../../../qtcreator.pri)

osx:  DESTDIR = $$IDE_LIBEXEC_PATH/qmldesigner
else: DESTDIR = $$IDE_LIBEXEC_PATH

include(../../../rpath.pri)

include(../../../../share/qtcreator/qml/qmlpuppet/qml2puppet/qml2puppet.pri)

isEmpty(PRECOMPILED_HEADER):PRECOMPILED_HEADER = $$PWD/../../../shared/qtcreator_pch.h


