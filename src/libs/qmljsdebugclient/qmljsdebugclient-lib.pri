contains(CONFIG, dll) {
    DEFINES += QMLJSDEBUGCLIENT_LIB
} else {
    DEFINES += QMLJSDEBUGCLIENT_STATIC_LIB
}

INCLUDEPATH += $$PWD/..

HEADERS += \
    $$PWD/qdeclarativedebugclient.h \
    $$PWD/qdeclarativeenginedebug.h \
    $$PWD/qdeclarativeoutputparser.h \
    $$PWD/qmljsdebugclient_global.h \
    $$PWD/qmlprofilereventlist.h \
    $$PWD/qmlprofilereventtypes.h \
    $$PWD/qmlprofilertraceclient.h \
    $$PWD/qpacketprotocol.h \
    $$PWD/qv8profilerclient.h \
    $$PWD/qmljsdebugclientconstants.h \
    $$PWD/qdebugmessageclient.h

SOURCES += \
    $$PWD/qdeclarativedebugclient.cpp \
    $$PWD/qdeclarativeenginedebug.cpp \
    $$PWD/qdeclarativeoutputparser.cpp \
    $$PWD/qmlprofilereventlist.cpp \
    $$PWD/qmlprofilertraceclient.cpp \
    $$PWD/qpacketprotocol.cpp \
    $$PWD/qv8profilerclient.cpp \
    $$PWD/qdebugmessageclient.cpp

OTHER_FILES += \
    $$PWD/qmljsdebugclient.pri \
    $$PWD/qmljsdebugclient-lib.pri

