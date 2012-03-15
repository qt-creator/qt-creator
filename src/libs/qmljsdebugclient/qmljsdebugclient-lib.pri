contains(CONFIG, dll) {
    DEFINES += QMLJSDEBUGCLIENT_LIB
} else {
    DEFINES += QMLJSDEBUGCLIENT_STATIC_LIB
}

INCLUDEPATH += $$PWD/..

HEADERS += \
    $$PWD/qmlprofilereventlocation.h \
    $$PWD/qdeclarativedebugclient.h \
    $$PWD/qmlenginedebugclient.h \
    $$PWD/qdeclarativeengineclient.h \
    $$PWD/qdeclarativeoutputparser.h \
    $$PWD/qmljsdebugclient_global.h \
    $$PWD/qmlprofilereventtypes.h \
    $$PWD/qmlprofilertraceclient.h \
    $$PWD/qpacketprotocol.h \
    $$PWD/qv8profilerclient.h \
    $$PWD/qmljsdebugclientconstants.h \
    $$PWD/qdebugmessageclient.h \
    $$PWD/qmldebuggerclient.h

SOURCES += \
    $$PWD/qdeclarativedebugclient.cpp \
    $$PWD/qmlenginedebugclient.cpp \
    $$PWD/qdeclarativeoutputparser.cpp \
    $$PWD/qmlprofilertraceclient.cpp \
    $$PWD/qpacketprotocol.cpp \
    $$PWD/qv8profilerclient.cpp \
    $$PWD/qdebugmessageclient.cpp \
    $$PWD/qmldebuggerclient.cpp

OTHER_FILES += \
    $$PWD/qmljsdebugclient.pri \
    $$PWD/qmljsdebugclient-lib.pri

