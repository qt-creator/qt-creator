TEMPLATE = app
QT += declarative

include(qml.pri)

SOURCES += main.cpp 
include(../../../../qtcreator.pri)

include(../../../libs/qmljsdebugger/qmljsdebugger.pri)
include(../../../libs/utils/utils.pri)
mac {
    qmljsLibraryTarget = $$qtLibraryName(QmlJSDebugger)
    utilsLibraryTarget = $$qtLibraryName(Utils)
}


include(../../../private_headers.pri)
DESTDIR = $$IDE_BIN_PATH
include(../../../rpath.pri)

mac {
    QMAKE_INFO_PLIST=Info_mac.plist
    TARGET=QMLObserver
    ICON=qml.icns
    QMAKE_POST_LINK=install_name_tool -change @executable_path/../PlugIns/lib$${qmljsLibraryTarget}.1.dylib @executable_path/../../../../PlugIns/lib$${qmljsLibraryTarget}.1.dylib \'$$DESTDIR/$${TARGET}.app/Contents/MacOS/$$TARGET\' \
                 && install_name_tool -change @executable_path/../PlugIns/lib$${utilsLibraryTarget}.1.dylib @executable_path/../../../../PlugIns/lib$${utilsLibraryTarget}.1.dylib \'$$DESTDIR/$${TARGET}.app/Contents/MacOS/$$TARGET\'
} else {
    TARGET=qmlobserver
}

