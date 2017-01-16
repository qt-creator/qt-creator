shared {
    DEFINES += QMLJS_LIBRARY
} else {
    DEFINES += QML_BUILD_STATIC_LIB
}

include(parser/parser.pri)

HEADERS += \
    $$PWD/qmljs_global.h \
    $$PWD/qmljsbind.h \
    $$PWD/qmljsbundle.h \
    $$PWD/qmljsevaluate.h \
    $$PWD/qmljsfindexportedcpptypes.h \
    $$PWD/qmljsdocument.h \
    $$PWD/qmljsscanner.h \
    $$PWD/qmljsinterpreter.h \
    $$PWD/qmljslink.h \
    $$PWD/qmljscheck.h \
    $$PWD/qmljsscopebuilder.h \
    $$PWD/qmljslineinfo.h \
    $$PWD/qmljscompletioncontextfinder.h \
    $$PWD/qmljsmodelmanagerinterface.h \
    $$PWD/qmljsicontextpane.h \
    $$PWD/qmljspropertyreader.h \
    $$PWD/qmljsrewriter.h \
    $$PWD/qmljsicons.h \
    $$PWD/qmljsplugindumper.h \
    $$PWD/qmljstypedescriptionreader.h \
    $$PWD/qmljsscopeastpath.h \
    $$PWD/qmljsvalueowner.h \
    $$PWD/qmljscontext.h \
    $$PWD/qmljsscopechain.h \
    $$PWD/qmljsutils.h \
    $$PWD/qmljsstaticanalysismessage.h \
    $$PWD/jsoncheck.h \
    $$PWD/qmljssimplereader.h \
    $$PWD/persistenttrie.h \
    $$PWD/qmljsqrcparser.h \
    $$PWD/qmljsconstants.h \
    $$PWD/qmljsimportdependencies.h \
    $$PWD/qmljsviewercontext.h \
    $$PWD/qmljsdescribevalue.h \
    $$PWD/qmljsdialect.h

SOURCES += \
    $$PWD/qmljsbind.cpp \
    $$PWD/qmljsbundle.cpp \
    $$PWD/qmljsevaluate.cpp \
    $$PWD/qmljsfindexportedcpptypes.cpp \
    $$PWD/qmljsdocument.cpp \
    $$PWD/qmljsscanner.cpp \
    $$PWD/qmljsinterpreter.cpp \
    $$PWD/qmljslink.cpp \
    $$PWD/qmljscheck.cpp \
    $$PWD/qmljsscopebuilder.cpp \
    $$PWD/qmljslineinfo.cpp \
    $$PWD/qmljscompletioncontextfinder.cpp \
    $$PWD/qmljsmodelmanagerinterface.cpp \
    $$PWD/qmljspropertyreader.cpp \
    $$PWD/qmljsrewriter.cpp \
    $$PWD/qmljsicons.cpp \
    $$PWD/qmljsplugindumper.cpp \
    $$PWD/qmljstypedescriptionreader.cpp \
    $$PWD/qmljsscopeastpath.cpp \
    $$PWD/qmljsvalueowner.cpp \
    $$PWD/qmljscontext.cpp \
    $$PWD/qmljsscopechain.cpp \
    $$PWD/qmljsutils.cpp \
    $$PWD/qmljsstaticanalysismessage.cpp \
    $$PWD/jsoncheck.cpp \
    $$PWD/qmljssimplereader.cpp \
    $$PWD/persistenttrie.cpp \
    $$PWD/qmljsqrcparser.cpp \
    $$PWD/qmljsimportdependencies.cpp \
    $$PWD/qmljsviewercontext.cpp \
    $$PWD/qmljsdescribevalue.cpp \
    $$PWD/qmljsdialect.cpp

DISTFILES += \
    $$PWD/parser/qmljs.g

contains(QT, gui) {
    SOURCES += \
        $$PWD/qmljsindenter.cpp \
        $$PWD/qmljscodeformatter.cpp \
        $$PWD/qmljsreformatter.cpp

    HEADERS += \
        $$PWD/qmljsindenter.h \
        $$PWD/qmljscodeformatter.h \
        $$PWD/qmljsreformatter.h
}

QT += xml
