#-------------------------------------------------
#
# Project created by QtCreator 2009-05-05T11:16:25
#
#-------------------------------------------------

TARGET = dumpertest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
../gdbmacros.cpp

exists($$QMAKE_INCDIR_QT/QtCore/private/qobject_p.h) {
   DEFINES+=HAS_QOBJECT_P_H
}

