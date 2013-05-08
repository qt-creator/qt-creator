QT += gui network

include(../../qtcreatorlibrary.pri)
include(utils-lib.pri)

lessThan(QT_MAJOR_VERSION, 5) {
#   Needed for QtCore/private/qwineventnotifier_p.h
    win32:include(../../private_headers.pri)
    linux-*: DEFINES += QTC_USE_QX11INFO
} else:linux-* {
    !isEmpty(QT.x11extras.name) {
        QT += x11extras
        DEFINES += QTC_USE_QX11INFO
    } else {
        warning("x11extras module not found, raising the main window might not work. " \
                "(The x11extras module is part of Qt 5.1 and later.)")
    }
}

win32: LIBS += -luser32
# PortsGatherer
win32: LIBS += -liphlpapi -lws2_32
