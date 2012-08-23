TEMPLATE = lib
TARGET = QmlEditorWidgets

DEFINES += QWEAKPOINTER_ENABLE_ARROW

unix:QMAKE_CXXFLAGS_DEBUG += -O3

include(../../qtcreatorlibrary.pri)
include(../qmljs/qmljs.pri)
include(../utils/utils.pri)
include(qmleditorwidgets-lib.pri)

