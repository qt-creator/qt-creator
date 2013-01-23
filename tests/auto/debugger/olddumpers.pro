greaterThan(QT_MAJOR_VERSION, 4): QT += core-private
include(../qttest.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)

DEBUGGERDIR = $$IDE_SOURCE_TREE/src/plugins/debugger
UTILSDIR    = $$IDE_SOURCE_TREE/src/libs/utils
MACROSDIR   = $$IDE_SOURCE_TREE/share/qtcreator/dumper

SOURCES += \
    $$DEBUGGERDIR/gdb/gdbmi.cpp \
    $$MACROSDIR/dumper.cpp \
    tst_olddumpers.cpp

exists($$QMAKE_INCDIR_QT/QtCore/private/qobject_p.h):DEFINES += USE_PRIVATE

DEFINES += MACROSDEBUG

win32:DEFINES += _CRT_SECURE_NO_WARNINGS

DEFINES -= QT_USE_FAST_CONCATENATION QT_USE_FAST_OPERATOR_PLUS
DEFINES -= QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

INCLUDEPATH += $$DEBUGGERDIR $$UTILSDIR $$MACROSDIR
