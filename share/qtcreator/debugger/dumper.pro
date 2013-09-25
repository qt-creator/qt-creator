TEMPLATE = lib
CONFIG += shared
linux-* {
    CONFIG -= release
    CONFIG += debug
}

HEADERS += dumper.h
SOURCES = dumper.cpp

false {
    DEFINES += USE_QT_GUI=0
    DEFINES += USE_QT_CORE=0
    QT=
    # We need a few convenience macros.
    #CONFIG -= qt
}
false {
    DEFINES += USE_QT_CORE=1
    DEFINES += USE_QT_GUI=0
    QT = core
    exists($$QMAKE_INCDIR_QT/QtCore/private/qobject_p.h):DEFINES += HAS_QOBJECT_P_H
}
true {
    DEFINES += USE_QT_CORE=1
    DEFINES += USE_QT_GUI=1
    QT = core gui
    greaterThan(QT_MAJOR_VERSION, 4):QT *= widgets
    exists($$QMAKE_INCDIR_QT/QtCore/private/qobject_p.h):DEFINES += HAS_QOBJECT_P_H
}
