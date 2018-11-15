shared {
    DEFINES += QMLDEBUG_LIBRARY
} else {
    DEFINES += QMLDEBUG_STATIC_LIB
}

HEADERS += \
    $$PWD/qmldebugclient.h \
    $$PWD/baseenginedebugclient.h \
    $$PWD/qmloutputparser.h \
    $$PWD/qmldebug_global.h \
    $$PWD/qpacketprotocol.h \
    $$PWD/qmldebugconstants.h \
    $$PWD/qdebugmessageclient.h \
    $$PWD/qmlenginedebugclient.h \
    $$PWD/basetoolsclient.h \
    $$PWD/qmltoolsclient.h \
    $$PWD/qmlenginecontrolclient.h \
    $$PWD/qmldebugcommandlinearguments.h \
    $$PWD/qmldebugconnection.h \
    $$PWD/qmldebugconnectionmanager.h

SOURCES += \
    $$PWD/qmldebugclient.cpp \
    $$PWD/baseenginedebugclient.cpp \
    $$PWD/qmloutputparser.cpp \
    $$PWD/qpacketprotocol.cpp \
    $$PWD/qdebugmessageclient.cpp \
    $$PWD/basetoolsclient.cpp \
    $$PWD/qmltoolsclient.cpp \
    $$PWD/qmlenginecontrolclient.cpp \
    $$PWD/qmldebugconnection.cpp \
    $$PWD/qmldebugconnectionmanager.cpp
