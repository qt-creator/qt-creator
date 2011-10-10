TEMPLATE = lib
CONFIG += shared
linux-* {
    CONFIG -= release
    CONFIG += debug
}
SOURCES = dumper.cpp
false {
    DEFINES += USE_QT_GUI=0
    QT = core
}
else {
    DEFINES += USE_QT_GUI=1
    QT = core \
        gui
    greaterThan(QT_MAJOR_VERSION, 4):QT *= widgets
}
exists($$QMAKE_INCDIR_QT/QtCore/private/qobject_p.h):DEFINES += HAS_QOBJECT_P_H
HEADERS += dumper.h
