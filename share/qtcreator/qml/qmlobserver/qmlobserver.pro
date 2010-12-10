TEMPLATE = app

CONFIG += qt uic

### FIXME: only debug plugins are now supported.
CONFIG -= release
CONFIG += debug

include(qml.pri)

SOURCES += main.cpp

exists($$PWD/qmljsdebugger/qmljsdebugger-lib.pri) {
# for building helpers within QT_INSTALL_DATA, we deploy the lib inside the observer directory.
    include($$PWD/qmljsdebugger/qmljsdebugger-lib.pri)
} else {
    include($$PWD/../qmljsdebugger/qmljsdebugger-lib.pri)
}

#INCLUDEPATH += ../../include/QtDeclarative
#INCLUDEPATH += ../../src/declarative/util
#INCLUDEPATH += ../../src/declarative/graphicsitems

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target

wince* {
    QT += xml

    contains(QT_CONFIG, scripttools) {
        QT += scripttools
    }
    contains(QT_CONFIG, phonon) {
        QT += phonon
    }
    contains(QT_CONFIG, xmlpatterns) {
        QT += xmlpatterns
    }
    contains(QT_CONFIG, webkit) {
        QT += webkit
    }
}
maemo5 {
    QT += maemo5
}
symbian {
    TARGET.UID3 = 0x20021317
    include($$QT_SOURCE_TREE/demos/symbianpkgrules.pri)
    TARGET.EPOCHEAPSIZE = 0x20000 0x4000000
    TARGET.CAPABILITY = NetworkServices ReadUserData
    !contains(S60_VERSION, 3.1):!contains(S60_VERSION, 3.2) {
        LIBS += -lsensrvclient -lsensrvutil
    }
    contains(QT_CONFIG, s60): {
        LIBS += -lavkon -lcone
    }
}
mac {
    QMAKE_INFO_PLIST=Info_mac.plist
    TARGET=QMLObserver
    ICON=qml.icns
} else {
    TARGET=qmlobserver
}
