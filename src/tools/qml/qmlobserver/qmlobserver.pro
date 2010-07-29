TEMPLATE = app
QT += declarative

include(qml.pri)

SOURCES += main.cpp 

# hack to get qtLibraryTarget macro working
TEMPLATE +=lib
include(../../../libs/qmljsdebugger/qmljsdebugger.pri)
mac {
    libraryTarget = $$qtLibraryTarget(QmlJSDebugger)
}
TEMPLATE -=lib

include(../../../../qtcreator.pri)
include(../../../private_headers.pri)
DESTDIR = $$IDE_BIN_PATH
include(../../../rpath.pri)

mac {
    QMAKE_INFO_PLIST=Info_mac.plist
    TARGET=QMLObserver
    ICON=qml.icns
    QMAKE_POST_LINK=install_name_tool -change @executable_path/../PlugIns/lib$${libraryTarget}.1.dylib @executable_path/../../../../PlugIns/lib$${libraryTarget}.1.dylib \'$$DESTDIR/$${TARGET}.app/Contents/MacOS/$$TARGET\'
} else {
    TARGET=qmlobserver
}

