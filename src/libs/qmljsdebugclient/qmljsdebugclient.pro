TEMPLATE = lib
TARGET = QmlJSDebugClient
QT += network
DEFINES += QMLJSDEBUGCLIENT_LIBRARY

include(../../qtcreatorlibrary.pri)
include(../symbianutils/symbianutils.pri)
include(qmljsdebugclient-lib.pri)

OTHER_FILES += \
    qmljsdebugclient.pri \
    qmljsdebugclient-lib.pri
