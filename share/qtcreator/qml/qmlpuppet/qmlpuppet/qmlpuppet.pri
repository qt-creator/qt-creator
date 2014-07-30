QT += core gui network declarative

greaterThan(QT_MAJOR_VERSION, 4): QT += declarative-private core-private widgets-private gui-private script-private

greaterThan(QT_MAJOR_VERSION, 4) {
} else {
    contains (QT_CONFIG, webkit) {
        QT += webkit
    }
}

DEFINES += QWEAKPOINTER_ENABLE_ARROW

include (../instances/instances.pri)
include (instances/instances.pri)
include (../commands/commands.pri)
include (../container/container.pri)
include (../interfaces/interfaces.pri)
include (../types/types.pri)

SOURCES += $$PWD/qmlpuppetmain.cpp
RESOURCES += $$PWD/../qmlpuppet.qrc
DEFINES -= QT_NO_CAST_FROM_ASCII

OTHER_FILES += Info.plist
macx {
    CONFIG -= app_bundle
    QMAKE_LFLAGS += -sectcreate __TEXT __info_plist \"$$PWD/Info.plist\"
} else {
    target.path  = $$QTC_PREFIX/bin
    INSTALLS    += target
}
