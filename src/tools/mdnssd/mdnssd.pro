QT       -= gui
QT       -= core
TEST = 0
include(../../../qtcreator.pri)
CONFIG -= console testlib TEST
TARGET = mdnssd
CONFIG   -= app_bundle

TEMPLATE = app

DESTDIR = $$IDE_BIN_PATH

DEFINES += PID_FILE=\\\"/tmp/mdnsd.pid\\\" MDNS_UDS_SERVERPATH=\\\"/tmp/mdnsd\\\" MDNS_DEBUGMSGS=0

SOURCES += \
    uds_daemon.c \
    uDNS.c \
    mDNSDebug.c \
    mDNS.c \
    GenLinkedList.c \
    dnssd_ipc.c \
    DNSDigest.c \
    DNSCommon.c

HEADERS += \
    uds_daemon.h \
    uDNS.h \
    mDNSUNP.h \
    mDNSEmbeddedAPI.h \
    mDNSDebug.h \
    GenLinkedList.h \
    dnssd_ipc.h \
    DNSCommon.h \
    DebugServices.h \
    dns_sd.h

linux-* {
SOURCES += mDNSPosix.c \
    PlatformCommon.c \
    PosixDaemon.c \
    mDNSUNP.c

HEADERS +=\
    PlatformCommon.h \
    mDNSPosix.h
}

*-g++ {
    QMAKE_CFLAGS += -Wno-unused-but-set-variable -Wno-strict-aliasing
    QMAKE_CXXFLAGS += -Wno-unused-but-set-variable
}

linux-* {
DEFINES += _GNU_SOURCE HAVE_IPV6 NOT_HAVE_SA_LEN USES_NETLINK HAVE_LINUX TARGET_OS_LINUX
}

macx {
DEFINES += HAVE_IPV6 __MAC_OS_X_VERSION_MIN_REQUIRED=__MAC_OS_X_VERSION_10_4 __APPLE_USE_RFC_2292
}

win32 {
    HEADERS += \
        CommonServices.h \
        DebugServices.h \
        Firewall.h \
        mDNSWin32.h \
        Poll.h \
        resource.h \
        Secret.h \
        Service.h \
        RegNames.h

    SOURCES += \
        DebugServices.c \
        Firewall.cpp \
        LegacyNATTraversal.c \
        main.c \
        mDNSWin32.c \
        Poll.c \
        Secret.c \
        Service.c

    RC_FILE = Service.rc

    MC_FILES += \
        EventLog.mc

    OTHER_FILES += \
        $$MC_FILES \
        Service.rc

    DEFINES += HAVE_IPV6 _WIN32_WINNT=0x0501 NDEBUG MDNS_DEBUGMSGS=0 TARGET_OS_WIN32 WIN32_LEAN_AND_MEAN USE_TCP_LOOPBACK PLATFORM_NO_STRSEP PLATFORM_NO_EPIPE PLATFORM_NO_RLIMIT UNICODE _UNICODE _CRT_SECURE_NO_DEPRECATE _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1 _LEGACY_NAT_TRAVERSAL_
    LIBS += ws2_32.lib advapi32.lib ole32.lib oleaut32.lib iphlpapi.lib netapi32.lib user32.lib powrprof.lib shell32.lib

    mc.output = ${QMAKE_FILE_BASE}.h
    mc.commands = mc ${QMAKE_FILE_NAME}
    mc.input = MC_FILES
    mc.CONFIG += no_link target_predeps explicit_dependencies
    QMAKE_EXTRA_COMPILERS += mc
}

target.path=/bin
INSTALLS+=target
