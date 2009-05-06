include(../../qtcreator.pri)
include(../shared/qtsingleapplication/qtsingleapplication.pri)

TEMPLATE = app
TARGET = $$IDE_APP_TARGET
DESTDIR = ../../bin


SOURCES += main.cpp

include(../rpath.pri)

win32 {
        RC_FILE = qtcreator.rc
}

macx {
        ICON = qtcreator.icns
        QMAKE_INFO_PLIST = Info.plist
}

macx {
    CONFIG(debug, debug|release):LIBS *= -lExtensionSystem_debug -lAggregation_debug
    else:LIBS *= -lExtensionSystem -lAggregation
}
win32 {
    CONFIG(debug, debug|release):LIBS *= -lExtensionSystemd -lAggregationd
    else:LIBS *= -lExtensionSystem -lAggregation
}
unix:!macx {
    LIBS *= -lExtensionSystem -lAggregation

    target.path  = /bin
    INSTALLS    += target
}
