TARGET = qml2puppet

TEMPLATE = app

include(../../../../qtcreator.pri)


CONFIG(debug) {
    DESTDIR = $$[QT_INSTALL_BINS]
} else {
    DESTDIR = $$IDE_BIN_PATH
}
include(../../../rpath.pri)

include(../../../../share/qtcreator/qml/qmlpuppet/qml2puppet/qml2puppet.pri)

isEmpty(PRECOMPILED_HEADER):PRECOMPILED_HEADER = $$PWD/../../../shared/qtcreator_pch.h
