
QT += core gui qml quick network v8
contains (QT_CONFIG, webkit) {
    QT += webkit
}

QT += core-private qml-private quick-private gui-private script-private v8-private

DEFINES += QWEAKPOINTER_ENABLE_ARROW

include (../instances/instances.pri)
include (instances/instances.pri)
include (../commands/commands.pri)
include (../container/container.pri)
include (../interfaces/interfaces.pri)

QT_BREAKPAD_ROOT_PATH = $$(QT_BREAKPAD_ROOT_PATH)
!isEmpty(QT_BREAKPAD_ROOT_PATH) {
    include($$QT_BREAKPAD_ROOT_PATH/qtbreakpad.pri)
}

SOURCES +=  $$PWD/main.cpp
RESOURCES +=  $$PWD/../qmlpuppet.qrc
DEFINES -= QT_NO_CAST_FROM_ASCII

OTHER_FILES += Info.plist.in
macx {
    info.input = Info.plist.in
    info.output = $$DESTDIR/$${TARGET}.app/Contents/Info.plist
    QMAKE_SUBSTITUTES += info
} else {
    target.path  = $$QTC_PREFIX/bin
    INSTALLS    += target
}
