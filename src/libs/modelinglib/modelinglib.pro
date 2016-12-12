
include(../../qtcreatorlibrary.pri)

!isEmpty(QT.svg.name): QT += svg
else: DEFINES += QT_NO_SVG

DEFINES += MODELING_LIBRARY

INCLUDEPATH += $$PWD $$PWD/qtserialization/inc

include(qstringparser/qstringparser.pri)
include(qtserialization/qtserialization.pri)
include(qmt/qmt.pri)
