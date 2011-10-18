#-------------------------------------------------
#
# Project created by QtCreator 2011-10-14T10:22:27
#
#-------------------------------------------------

QT       -= gui
QT       += core

TARGET = mdnssd
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

include(../../../qtcreator.pri)
DESTDIR = $$IDE_BIN_PATH

DEFINES += PID_FILE=\\\"/var/run/mdnsd.pid\\\" MDNS_UDS_SERVERPATH=\\\"/var/run/mdnsd\\\" MDNS_DEBUGMSGS=0

SOURCES += \
    uds_daemon.c \
    uDNS.c \
    PosixDaemon.c \
    PlatformCommon.c \
    mDNSUNP.c \
    mDNSPosix.c \
    mDNSDebug.c \
    mDNS.c \
    GenLinkedList.c \
    dnssd_ipc.c \
    DNSDigest.c \
    DNSCommon.c

HEADERS += \
    uds_daemon.h \
    uDNS.h \
    PlatformCommon.h \
    mDNSUNP.h \
    mDNSPosix.h \
    mDNSEmbeddedAPI.h \
    mDNSDebug.h \
    GenLinkedList.h \
    dnssd_ipc.h \
    DNSCommon.h \
    DebugServices.h \
    dns_sd.h

linux-* {
DEFINES += NOT_HAVE_SA_LEN USES_NETLINK HAVE_LINUX TARGET_OS_LINUX
}
macx {
DEFINES += HAVE_IPV6 __MAC_OS_X_VERSION_MIN_REQUIRED=__MAC_OS_X_VERSION_10_4 __APPLE_USE_RFC_2292
}

