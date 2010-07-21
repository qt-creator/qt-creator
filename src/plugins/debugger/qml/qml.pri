include(../../../private_headers.pri)
exists($${QT_PRIVATE_HEADERS}/QtDeclarative/private/qdeclarativecontext_p.h) {
    HEADERS += \
        $$PWD/qmlengine.h \
        $$PWD/canvasframerate.h \

    SOURCES += \
        $$PWD/qmlengine.cpp \
        $$PWD/canvasframerate.cpp \

    QT += declarative
}
