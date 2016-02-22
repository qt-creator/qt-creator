
include(../../qtcreatorlibrary.pri)

QT += svg

DEFINES += QMT_LIBRARY

INCLUDEPATH += $$PWD $$PWD/qtserialization/inc

include(qstringparser/qstringparser.pri)
include(qtserialization/qtserialization.pri)
include(qmt/qmt.pri)
