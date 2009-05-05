TEMPLATE = lib
CONFIG += shared
linux-* {
CONFIG -= release
CONFIG += debug
}
SOURCES=gdbmacros.cpp

exists($$QMAKE_INCDIR_QT/QtCore/private/qobject_p.h) {
   DEFINES+=HAS_QOBJECT_P_H
}
