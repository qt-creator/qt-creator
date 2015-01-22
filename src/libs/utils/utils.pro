QT += gui network

include(../../qtcreatorlibrary.pri)
include(utils-lib.pri)

linux-* {
    !isEmpty(QT.x11extras.name) {
        QT += x11extras
        CONFIG += x11
        DEFINES += QTC_USE_QX11INFO
    } else {
        warning("x11extras module not found, raising the main window might not work. " \
                "(The x11extras module is part of Qt 5.1 and later.)")
    }
}

win32: LIBS += -luser32 -lshell32
# PortsGatherer
win32: LIBS += -liphlpapi -lws2_32
