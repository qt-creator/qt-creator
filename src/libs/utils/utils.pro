TEMPLATE = lib
TARGET = Utils
QT += gui network

include(../../qtcreatorlibrary.pri)
include(utils_dependencies.pri)

include(utils-lib.pri)

lessThan(QT_MAJOR_VERSION, 5) {
#   Needed for QtCore/private/qwineventnotifier_p.h
    win32:include(../../private_headers.pri)
}

win32: LIBS += -luser32
# PortsGatherer
win32: LIBS += -liphlpapi -lws2_32
