QT += declarative script
INCLUDEPATH += $$PWD $$PWD/include editor
DEPENDPATH += $$PWD $$PWD/include editor

contains(CONFIG, dll) {
    DEFINES += BUILD_QMLJSDEBUGGER_LIB
} else {
    DEFINES += BUILD_QMLJSDEBUGGER_STATIC_LIB
}

## Once is not enough
include($$PWD/../../private_headers.pri)
include($$PWD/../../private_headers.pri)

include($$PWD/editor/editor.pri)

## Input
HEADERS += \
    include/jsdebuggeragent.h \
    include/qdeclarativedesignview.h \
    include/qdeclarativedesigndebugserver.h \
    include/crumblepath.h \
    include/qmlviewerconstants.h \
    include/qmljsdebugger_global.h \
    qdeclarativedesignview_p.h

SOURCES += \
    jsdebuggeragent.cpp \
    qdeclarativedesignview.cpp \
    qdeclarativedesigndebugserver.cpp

OTHER_FILES += qmljsdebugger.pri
