TEMPLATE = lib
TARGET = QmlEditorWidgets

DEFINES += QWEAKPOINTER_ENABLE_ARROW

unix:QMAKE_CXXFLAGS_DEBUG += -O3

include(../../qtcreatorlibrary.pri)
include(qmleditorwidgets_dependencies.pri)
include(qmleditorwidgets-lib.pri)

