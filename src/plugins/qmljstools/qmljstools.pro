TEMPLATE = lib
TARGET = QmlJSTools
include(../../qtcreatorplugin.pri)
include(qmljstools_dependencies.pri)

# DEFINES += QT_NO_CAST_FROM_ASCII
DEFINES += QT_NO_CAST_TO_ASCII
DEFINES += QMLJSTOOLS_LIBRARY

include(qmljstools-lib.pri)
