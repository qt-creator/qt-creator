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
    $$PWD/qmljslink.h \
    $$PWD/qmljscheck.h \
    $$PWD/qmljsscopebuilder.h \
    $$PWD/qmljslookupcontext.h \
    $$PWD/qmljslineinfo.h \
    $$PWD/qmljscompletioncontextfinder.h \
    $$PWD/qmljsmodelmanagerinterface.h \
    $$PWD/qmljsicontextpane.h \
    $$PWD/qmljspropertyreader.h \
    $$PWD/qmljsrewriter.h \
    $$PWD/qmljsicons.h \
    $$PWD/qmljsdelta.h \
    $$PWD/qmljstypedescriptionreader.h

SOURCES += \
    $$PWD/qmljsbind.cpp \
    $$PWD/qmljsevaluate.cpp \
    $$PWD/qmljsdocument.cpp \
    $$PWD/qmljsscanner.cpp \
    $$PWD/qmljsinterpreter.cpp \
    $$PWD/qmljslink.cpp \
    $$PWD/qmljscheck.cpp \
    $$PWD/qmljsscopebuilder.cpp \
    $$PWD/qmljslookupcontext.cpp \
    $$PWD/qmljslineinfo.cpp \
    $$PWD/qmljscompletioncontextfinder.cpp \
    $$PWD/qmljsmodelmanagerinterface.cpp \
    $$PWD/qmljspropertyreader.cpp \
    $$PWD/qmljsrewriter.cpp \
    $$PWD/qmljsicons.cpp \
    $$PWD/qmljsdelta.cpp \
    $$PWD/qmljstypedescriptionreader.cpp

RESOURCES += \
    $$PWD/qmljs.qrc

OTHER_FILES += \
    $$PWD/parser/qmljs.g

contains(QT, gui) {
    SOURCES += \
        $$PWD/qmljsindenter.cpp \
        $$PWD/qmljscodeformatter.cpp

    HEADERS += \
        $$PWD/qmljsindenter.h \
        $$PWD/qmljscodeformatter.h
}
