contains(CONFIG, dll) {
    DEFINES += QMLJS_BUILD_DIR
} else {
    DEFINES += QML_BUILD_STATIC_LIB
}

include(parser/parser.pri)

DEFINES += QSCRIPTHIGHLIGHTER_BUILD_LIB

DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD/..

HEADERS += \
    $$PWD/qmljs_global.h \
    $$PWD/qmljsbind.h \
    $$PWD/qmljscheck.h \
    $$PWD/qmljsdocument.h \
    $$PWD/qmljsidcollector.h \
    $$PWD/qmljsmetatypebackend.h \
    $$PWD/qmljspackageinfo.h \
    $$PWD/qmljsscanner.h \
    $$PWD/qmljssymbol.h \
    $$PWD/qmljstypesystem.h \
    $$PWD/qmljsinterpreter.h

SOURCES += \
    $$PWD/qmljsbind.cpp \
    $$PWD/qmljscheck.cpp \
    $$PWD/qmljsdocument.cpp \
    $$PWD/qmljsidcollector.cpp \
    $$PWD/qmljsmetatypebackend.cpp \
    $$PWD/qmljspackageinfo.cpp \
    $$PWD/qmljsscanner.cpp \
    $$PWD/qmljssymbol.cpp \
    $$PWD/qmljstypesystem.cpp \
    $$PWD/qmljsinterpreter.cpp

contains(QT_CONFIG, declarative) {
    QT += declarative

    HEADERS += \
        $$PWD/qtdeclarativemetatypebackend.h

    SOURCES += \
        $$PWD/qtdeclarativemetatypebackend.cpp
} else {
    DEFINES += NO_DECLARATIVE_BACKEND
}

contains(QT, gui) {
    SOURCES += $$PWD/qmljshighlighter.cpp $$PWD/qmljsindenter.cpp
    HEADERS += $$PWD/qmljshighlighter.h $$PWD/qmljsindenter.h
}
