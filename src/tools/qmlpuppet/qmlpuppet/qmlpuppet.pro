TARGET = qmlpuppet

TEMPLATE = app

include(../../../../qtcreator.pri)
include(../../../private_headers.pri)
DESTDIR = $$IDE_BIN_PATH
include(../../../rpath.pri)

include(../../../../share/qtcreator/qml/qmlpuppet/qmlpuppet/qmlpuppet.pri)

QT_BREAKPAD_ROOT_PATH = $$(QT_BREAKPAD_ROOT_PATH)
!isEmpty(QT_BREAKPAD_ROOT_PATH) {
    include($$QT_BREAKPAD_ROOT_PATH/qtbreakpad.pri)
}