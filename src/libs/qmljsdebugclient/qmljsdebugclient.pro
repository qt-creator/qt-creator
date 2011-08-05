TEMPLATE = lib
CONFIG += dll
TARGET = QmlJSDebugClient

DEFINES += QMLJSDEBUGCLIENT_LIBRARY

include(../../qtcreatorlibrary.pri)
include(../symbianutils/symbianutils.pri)
include(qmljsdebugclient-lib.pri)

OTHER_FILES += \
    qmljsdebugclient.pri \
    qmljsdebugclient-lib.pri
