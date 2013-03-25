QT       -= gui
QT       += network
CONFIG   += exceptions

TARGET = zeroconf
TEMPLATE = lib

DEFINES += ZEROCONF_LIBRARY

SOURCES += servicebrowser.cpp \
    embeddedLib.cpp \
    mdnsderived.cpp \
    avahiLib.cpp \
    dnsSdLib.cpp

HEADERS += servicebrowser.h \
        zeroconf_global.h \
        dns_sd_types.h \
    servicebrowser_p.h \
    mdnsderived.h \
    syssocket.h

include(../../qtcreatorlibrary.pri)

win32{
    LIBS += -lws2_32
}
linux-* {
    DEFINES += _GNU_SOURCE HAVE_IPV6 USES_NETLINK HAVE_LINUX TARGET_OS_LINUX
}
