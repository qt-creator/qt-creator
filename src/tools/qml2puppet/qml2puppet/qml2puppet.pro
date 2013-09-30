TARGET = qml2puppet

TEMPLATE = app

include(../../../../qtcreator.pri)


BUILD_PUPPET_IN_CREATOR_BINPATH = $$(BUILD_PUPPET_IN_CREATOR_BINPATH)
CONFIG(debug):isEmpty(BUILD_PUPPET_IN_CREATOR_BINPATH) {
    DESTDIR = $$[QT_INSTALL_BINS]
} else {
    DESTDIR = $$IDE_BIN_PATH
    message("Build puppet in qtcreator bin path!")
}
include(../../../rpath.pri)

include(../../../../share/qtcreator/qml/qmlpuppet/qml2puppet/qml2puppet.pri)

isEmpty(PRECOMPILED_HEADER):PRECOMPILED_HEADER = $$PWD/../../../shared/qtcreator_pch.h


