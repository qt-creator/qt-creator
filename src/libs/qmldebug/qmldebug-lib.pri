contains(CONFIG, dll) {
    DEFINES += QMLDEBUG_LIB
} else {
    DEFINES += QMLDEBUG_STATIC_LIB
}

INCLUDEPATH += $$PWD/..

HEADERS += \
    $$PWD/qmlprofilereventlocation.h \
    $$PWD/qmldebugclient.h \
    $$PWD/baseenginedebugclient.h \
    $$PWD/declarativeenginedebugclient.h \
    $$PWD/qmloutputparser.h \
    $$PWD/qmldebug_global.h \
    $$PWD/qmlprofilereventtypes.h \
    $$PWD/qmlprofilertraceclient.h \
    $$PWD/qpacketprotocol.h \
    $$PWD/qv8profilerclient.h \
    $$PWD/qmldebugconstants.h \
    $$PWD/qdebugmessageclient.h \
    $$PWD/qmlenginedebugclient.h

SOURCES += \
    $$PWD/qmldebugclient.cpp \
    $$PWD/baseenginedebugclient.cpp \
    $$PWD/qmloutputparser.cpp \
    $$PWD/qmlprofilertraceclient.cpp \
    $$PWD/qpacketprotocol.cpp \
    $$PWD/qv8profilerclient.cpp \
    $$PWD/qdebugmessageclient.cpp \
    $$PWD/qmlenginedebugclient.cpp

