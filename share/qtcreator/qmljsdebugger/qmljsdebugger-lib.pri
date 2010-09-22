QT += declarative script
INCLUDEPATH += $$PWD $$PWD/include editor
DEPENDPATH += $$PWD $$PWD/include editor

contains(CONFIG, dll) {
    DEFINES += BUILD_QMLJSDEBUGGER_LIB
} else {
    DEFINES += BUILD_QMLJSDEBUGGER_STATIC_LIB
}

include($$PWD/private_headers.pri)

## Input
HEADERS += \
    include/jsdebuggeragent.h \
    include/qmljsdebugger_global.h

SOURCES += \
    jsdebuggeragent.cpp

contains(DEFINES, QMLOBSERVER) {
    include($$PWD/editor/editor.pri)

    HEADERS += \
        include/qdeclarativeviewobserver.h \
        include/qdeclarativeobserverservice.h \
        include/qmlobserverconstants.h \
        qdeclarativeviewobserver_p.h

    SOURCES += \
        qdeclarativeviewobserver.cpp \
        qdeclarativeobserverservice.cpp
}

OTHER_FILES += qmljsdebugger.pri
