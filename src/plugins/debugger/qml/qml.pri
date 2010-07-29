
HEADERS += $$PWD/qmlengine.h
SOURCES += $$PWD/qmlengine.cpp

include(../../../private_headers.pri)
exists($${QT_PRIVATE_HEADERS}/QtDeclarative/private/qdeclarativecontext_p.h) {
    HEADERS += \
        $$PWD/canvasframerate.h \

    SOURCES += \
        $$PWD/canvasframerate.cpp \

    QT += declarative
}
