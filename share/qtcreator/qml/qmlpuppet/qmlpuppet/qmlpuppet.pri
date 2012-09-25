QT += core gui network

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += quick1 quick1-private core-private widgets-private gui-private script-private
} else {
    QT += declarative
}

contains (QT_CONFIG, webkit) {
    QT += webkit
}

DEFINES += QWEAKPOINTER_ENABLE_ARROW

include (instances/instances.pri)
include (../instances/instances.pri)
include (../commands/commands.pri)
include (../container/container.pri)
include (../interfaces/interfaces.pri)

SOURCES += $$PWD/main.cpp
RESOURCES += $$PWD/../qmlpuppet.qrc

OTHER_FILES += Info.plist.in
macx {
    info.input = $$PWD/Info.plist.in
    info.output = $$DESTDIR/$${TARGET}.app/Contents/Info.plist
    QMAKE_SUBSTITUTES += info
} else {
    target.path  = $$QTC_PREFIX/bin
    INSTALLS    += target
}
