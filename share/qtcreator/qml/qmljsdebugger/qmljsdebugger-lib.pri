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
    $$PWD/include/jsdebuggeragent.h \
    $$PWD/include/qmljsdebugger_global.h

SOURCES += \
    $$PWD/jsdebuggeragent.cpp

contains(DEFINES, QMLOBSERVER) {
    include($$PWD/editor/editor.pri)

    HEADERS += \
        $$PWD/include/qdeclarativeviewobserver.h \
        $$PWD/include/qdeclarativeobserverservice.h \
        $$PWD/include/qmlobserverconstants.h \
        $$PWD/qdeclarativeviewobserver_p.h

    SOURCES += \
        $$PWD/qdeclarativeviewobserver.cpp \
        $$PWD/qdeclarativeobserverservice.cpp
}

OTHER_FILES += $$PWD/qmljsdebugger.pri
