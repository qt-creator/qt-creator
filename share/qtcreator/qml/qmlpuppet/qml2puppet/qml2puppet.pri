QT += core gui widgets qml quick network
QT += core-private qml-private quick-private gui-private

CONFIG += c++11

DEFINES -= QT_CREATOR

# This .pri file contains classes to enable a special multilanguage translator
MULTILANGUAGE_SUPPORT_PRI=$$(MULTILANGUAGE_SUPPORT_PRI)
!isEmpty(MULTILANGUAGE_SUPPORT_PRI) {
    exists($$(MULTILANGUAGE_SUPPORT_PRI)): message(including \"$$(MULTILANGUAGE_SUPPORT_PRI)\")
    else: error("MULTILANGUAGE_SUPPORT_PRI: \"$$(MULTILANGUAGE_SUPPORT_PRI)\" does not exist.")
    include($$(MULTILANGUAGE_SUPPORT_PRI))
    DEFINES += MULTILANGUAGE_TRANSLATIONPROVIDER
}

include (editor3d/editor3d.pri)
include (../instances/instances.pri)
include (instances/instances.pri)
include (../commands/commands.pri)
include (../container/container.pri)
include (../interfaces/interfaces.pri)
include (../types/types.pri)
include (../qmlprivategate/qmlprivategate.pri)
include (iconrenderer/iconrenderer.pri)

SOURCES +=  $$PWD/qml2puppetmain.cpp
RESOURCES +=  $$PWD/../qmlpuppet.qrc

DISTFILES += Info.plist

unix:!openbsd:!osx: LIBS += -lrt # posix shared memory

osx {
    CONFIG -= app_bundle
    QMAKE_LFLAGS += -Wl,-sectcreate,__TEXT,__info_plist,$$system_quote($$PWD/Info.plist)
}

osx:  target.path = $$INSTALL_LIBEXEC_PATH/qmldesigner
else: target.path = $$INSTALL_LIBEXEC_PATH
INSTALLS += target
