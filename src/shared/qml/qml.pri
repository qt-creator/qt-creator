include(parser/parser.pri)

DEPENDPATH += $$PWD $$PWD/metatype
INCLUDEPATH *= $$PWD/..

HEADERS += \
    $$PWD/qml_global.h \
    $$PWD/qmlidcollector.h \
    $$PWD/qmldocument.h \
    $$PWD/qmlpackageinfo.h \
    $$PWD/qmlsymbol.h \
    $$PWD/metatype/exception.h \
    $$PWD/metatype/QmlMetaTypeBackend.h \
    $$PWD/metatype/qmltypesystem.h

SOURCES += \
    $$PWD/qmlidcollector.cpp \
    $$PWD/qmldocument.cpp \
    $$PWD/qmlsymbol.cpp \
    $$PWD/qmlpackageinfo.cpp \
    $$PWD/metatype/exception.cpp \
    $$PWD/metatype/QmlMetaTypeBackend.cpp \
    $$PWD/metatype/qmltypesystem.cpp

contains(QT_CONFIG, declarative) {
    QT += declarative

    DEFINES += BUILD_DECLARATIVE_BACKEND

    HEADERS += \
        $$PWD/metatype/metainfo.h \
        $$PWD/metatype/nodemetainfo.h \
        $$PWD/metatype/propertymetainfo.h \
        $$PWD/metatype/QtDeclarativeMetaTypeBackend.h \
        $$PWD/metatype/invalidmetainfoexception.h

    SOURCES += \
        $$PWD/metatype/metainfo.cpp \
        $$PWD/metatype/nodemetainfo.cpp \
        $$PWD/metatype/propertymetainfo.cpp \
        $$PWD/metatype/QtDeclarativeMetaTypeBackend.cpp \
        $$PWD/metatype/invalidmetainfoexception.cpp
}
