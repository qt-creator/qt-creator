TARGET = qmlpuppet

TEMPLATE = app

QT += core gui declarative network webkit

DEFINES += QWEAKPOINTER_ENABLE_ARROW

include(../../../qtcreator.pri)
include(../../private_headers.pri)
DESTDIR = $$IDE_BIN_PATH
include(../../rpath.pri)

include (../../plugins/qmldesigner/designercore/instances/instances.pri)
include (../../plugins/qmldesigner/designercore/exceptions/exceptions.pri)


QT_BREAKPAD_ROOT_PATH = $$(QT_BREAKPAD_ROOT_PATH)
!isEmpty(QT_BREAKPAD_ROOT_PATH) {
    include($$QT_BREAKPAD_ROOT_PATH/qtbreakpad.pri)
}

SOURCES += main.cpp
RESOURCES += qmlpuppet.qrc

OTHER_FILES += Info.plist
macx:QMAKE_INFO_PLIST = Info.plist
