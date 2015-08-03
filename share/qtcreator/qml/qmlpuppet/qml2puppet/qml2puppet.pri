QT += core gui widgets qml quick network
QT += core-private qml-private quick-private gui-private

CONFIG += c++11

DEFINES -= QT_CREATOR

include (../instances/instances.pri)
include (instances/instances.pri)
include (../commands/commands.pri)
include (../container/container.pri)
include (../interfaces/interfaces.pri)
include (../types/types.pri)
include (../qmlprivategate/qmlprivategate.pri)

QT_BREAKPAD_ROOT_PATH = $$(QT_BREAKPAD_ROOT_PATH)
!isEmpty(QT_BREAKPAD_ROOT_PATH) {
    include($$QT_BREAKPAD_ROOT_PATH/qtbreakpad.pri)
}

SOURCES +=  $$PWD/qml2puppetmain.cpp
RESOURCES +=  $$PWD/../qmlpuppet.qrc
DEFINES -= QT_NO_CAST_FROM_ASCII

DISTFILES += Info.plist

unix:!osx:LIBS += -lrt # posix shared memory

osx {
    CONFIG -= app_bundle
    QMAKE_LFLAGS += -Wl,-sectcreate,__TEXT,__info_plist,$$system_quote($$PWD/Info.plist)
} else {
    target.path  = $$QTC_PREFIX/$$relative_path($$IDE_LIBEXEC_PATH, $$IDE_BUILD_TREE)
    INSTALLS    += target
}
