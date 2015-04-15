QT += gui network

include(../../qtcreatorlibrary.pri)
include(utils-lib.pri)

win32: LIBS += -luser32 -lshell32
# PortsGatherer
win32: LIBS += -liphlpapi -lws2_32
