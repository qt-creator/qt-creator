contains(CONFIG, dll) {
    DEFINES += QMLJSDEBUGCLIENT_LIB
} else {
    DEFINES += QMLJSDEBUGCLIENT_STATIC_LIB
}

INCLUDEPATH += $$PWD/..

HEADERS += \
    $$PWD/qdeclarativeenginedebug.h \
    $$PWD/qpacketprotocol.h \
    $$PWD/qdeclarativedebugclient.h \
    $$PWD/qmljsdebugclient_global.h

SOURCES += \
    $$PWD/qdeclarativeenginedebug.cpp \
    $$PWD/qpacketprotocol.cpp \
    $$PWD/qdeclarativedebugclient.cpp

OTHER_FILES += \
    $$PWD/qmljsdebugclient.pri \
    $$PWD/qmljsdebugclient-lib.pri
