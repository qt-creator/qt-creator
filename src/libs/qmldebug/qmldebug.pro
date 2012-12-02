TEMPLATE = lib
TARGET = QmlDebug
QT += network

include(../../qtcreatorlibrary.pri)
include(qmldebug-lib.pri)

DEFINES += QT_NO_CAST_FROM_ASCII

OTHER_FILES += \
    qmldebug.pri

