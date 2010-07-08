TEMPLATE = app
QT += declarative

include(qml.pri)

SOURCES += main.cpp 

include(../../../../qtcreator.pri)
include(../../../private_headers.pri)
include(../../../rpath.pri)

mac {
    QMAKE_INFO_PLIST=Info_mac.plist
    ICON=qml.icns
    TARGET=QMLViewer
    DESTDIR = $$IDE_APP_PATH
} else {
    TARGET=qmlviewer
    DESTDIR = $$IDE_BIN_PATH
}

