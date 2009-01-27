IDE_BUILD_TREE = $$OUT_PWD/../../

include(../qworkbench.pri)
include(../shared/qtsingleapplication/qtsingleapplication.pri)

macx {
    CONFIG(debug, debug|release):LIBS *= -lExtensionSystem_debug -lAggregation_debug
    else:LIBS *= -lExtensionSystem -lAggregation
}
win32 {
    CONFIG(debug, debug|release):LIBS *= -lExtensionSystemd -lAggregationd
    else:LIBS *= -lExtensionSystem -lAggregation
}
linux-* {
    LIBS *= -lExtensionSystem -lAggregation
    ISGCC33=$$(GCC33)
    !equals(ISGCC33, 1):QT += svg dbus

    target.path  = /bin
    INSTALLS    += target

}

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
}

include(../../share/share.pri)

