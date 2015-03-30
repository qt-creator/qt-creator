QT += core gui network declarative

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += declarative-private core-private widgets-private gui-private
} else {
    contains (QT_CONFIG, webkit) {
        QT += webkit
    }
}

DEFINES += QWEAKPOINTER_ENABLE_ARROW
# to support the in Qt5 deprecated but in Qt4 useful QWeakPointer
# change the QT_DISABLE_DEPRECATED_BEFORE version to Qt4
greaterThan(QT_MAJOR_VERSION, 4):DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x040900

include (../instances/instances.pri)
include (instances/instances.pri)
include (../commands/commands.pri)
include (../container/container.pri)
include (../interfaces/interfaces.pri)
include (../types/types.pri)

SOURCES += $$PWD/qmlpuppetmain.cpp
RESOURCES += $$PWD/../qmlpuppet.qrc
DEFINES -= QT_NO_CAST_FROM_ASCII

unix:!macx:LIBS += -lrt # posix shared memory

DISTFILES += Info.plist
macx {
    CONFIG -= app_bundle
    QMAKE_LFLAGS += -Wl,-sectcreate,__TEXT,__info_plist,\"$$PWD/Info.plist\"
} else {
    target.path  = $$QTC_PREFIX/bin
    INSTALLS    += target
}
