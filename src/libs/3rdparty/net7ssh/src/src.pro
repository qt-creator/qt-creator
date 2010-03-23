TEMPLATE = lib
TARGET = Net7ssh

CONFIG += dll

include(../../../../qtcreatorlibrary.pri)

DEPENDPATH += .
INCLUDEPATH += $$PWD $$PWD/../../botan $$PWD/../../botan/build

LIBS += -l$$qtLibraryTarget(Botan)

win32 {
    LIBS +=  -lWs2_32
    win32-msvc*: QMAKE_CXXFLAGS += -wd4250 -wd4251 -wd4290

    DEFINES += _CRT_SECURE_NO_WARNINGS
    DEFINES += NE7SSH_EXPORTS=1 _WINDLL _USRDLL _CONSOLE _WINDOWS
}

unix {
    QMAKE_CXXFLAGS_HIDESYMS -= -fvisibility-inlines-hidden # for ubuntu 7.04
    QMAKE_CXXFLAGS += -Wno-unused-parameter
}

# Input
HEADERS += crypt.h \
        ne7ssh.h \
        ne7ssh_channel.h \
        ne7ssh_connection.h \
        ne7ssh_error.h \
        ne7ssh_kex.h \
        ne7ssh_keys.h \
        ne7ssh_mutex.h \
        ne7ssh_session.h \
        ne7ssh_sftp.h \
        ne7ssh_sftp_packet.h \
        ne7ssh_string.h \
        ne7ssh_transport.h \
        ne7ssh_types.h

SOURCES += crypt.cpp \
        ne7ssh.cpp \
        ne7ssh_channel.cpp \
        ne7ssh_connection.cpp \
        ne7ssh_error.cpp \
        ne7ssh_kex.cpp \
        ne7ssh_keys.cpp \
        ne7ssh_mutex.cpp \
        ne7ssh_session.cpp \
        ne7ssh_sftp.cpp \
        ne7ssh_sftp_packet.cpp \
        ne7ssh_string.cpp \
        ne7ssh_transport.cpp
