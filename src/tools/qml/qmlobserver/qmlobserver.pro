TEMPLATE = app
QT += declarative

include(qml.pri)

SOURCES += main.cpp 

include(../../../../qtcreator.pri)
include(../../../private_headers.pri)
DESTDIR = $$IDE_BIN_PATH
include(../../../rpath.pri)

mac {
    QMAKE_INFO_PLIST=Info_mac.plist
    TARGET=QMLObserver
    ICON=qml.icns
} else {
    TARGET=qmlobserver
}
