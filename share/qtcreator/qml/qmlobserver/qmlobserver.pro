TEMPLATE = app

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

mac {
    QMAKE_INFO_PLIST=Info_mac.plist
    TARGET=QMLObserver
    ICON=qml.icns
} else {
    TARGET=qmlobserver
}

