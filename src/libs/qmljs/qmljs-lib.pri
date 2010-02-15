contains(CONFIG, dll) {
    DEFINES += QMLJS_BUILD_DIR
} else {
    DEFINES += QML_BUILD_STATIC_LIB
}

include(parser/parser.pri)

DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD/..

HEADERS += \
    $$PWD/qmljs_global.h \
    $$PWD/qmljsbind.h \
    $$PWD/qmljsevaluate.h \
    $$PWD/qmljsdocument.h \
    $$PWD/qmljsscanner.h \
    $$PWD/qmljsinterpreter.h \
    $$PWD/qmljslink.h

SOURCES += \
    $$PWD/qmljsbind.cpp \
    $$PWD/qmljsevaluate.cpp \
    $$PWD/qmljsdocument.cpp \
    $$PWD/qmljsscanner.cpp \
    $$PWD/qmljsinterpreter.cpp \
    $$PWD/qmljsmetatypesystem.cpp \
    $$PWD/qmljslink.cpp

contains(QT_CONFIG, declarative) {
    QT += declarative
} else {
    DEFINES += NO_DECLARATIVE_BACKEND
}

contains(QT, gui) {
    SOURCES += $$PWD/qmljsindenter.cpp
    HEADERS += $$PWD/qmljsindenter.h
}
