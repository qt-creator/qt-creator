TEMPLATE = lib
CONFIG += shared
linux-* {
    CONFIG -= release
    CONFIG += debug
}
SOURCES = gdbmacros.cpp
false {
    DEFINES += USE_QT_GUI=0
    QT = core
}
else {
    DEFINES += USE_QT_GUI=1
    QT = core \
        gui
}
exists($$QMAKE_INCDIR_QT/QtCore/private/qobject_p.h):DEFINES += HAS_QOBJECT_P_H
HEADERS += gdbmacros_p.h
