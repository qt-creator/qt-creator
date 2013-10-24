TARGET = qml2puppet

TEMPLATE = app

include(../../../../qtcreator.pri)

BUILD_PUPPET_IN_CREATOR_BINPATH = $$(BUILD_PUPPET_IN_CREATOR_BINPATH)
CONFIG(debug, debug|release):isEmpty(BUILD_PUPPET_IN_CREATOR_BINPATH) {
    QML_TARGET_PATH=$$[QT_INSTALL_BINS]/$$TARGET$$TARGET_EXT
    if(write_file($$QML_TARGET_PATH)) {
        CONVERTED_PATH=$$system_quote($$system_path($$QML_TARGET_PATH))
        win32 {
            system(del $$CONVERTED_PATH)
        } else {
            system(rm $$CONVERTED_PATH)
        }
        DESTDIR = $$[QT_INSTALL_BINS]
        message("Build Qml Puppet to the Qt binary directory!")
    } else {
        message("Cannot create write Qml Puppet to the Qt binary directory!")
    }
} else {
    DESTDIR = $$IDE_BIN_PATH
    message("Build Qml Puppet to Qt Creator binary directory!")
}
include(../../../rpath.pri)

include(../../../../share/qtcreator/qml/qmlpuppet/qml2puppet/qml2puppet.pri)

isEmpty(PRECOMPILED_HEADER):PRECOMPILED_HEADER = $$PWD/../../../shared/qtcreator_pch.h


