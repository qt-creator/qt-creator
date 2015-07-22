INCLUDEPATH += ../mockup

include(gmock_dependency.pri)
include(clang_dependency.pri)
include(creator_dependency.pri)

QT += core network testlib widgets
CONFIG += console c++14 testcase
CONFIG -= app_bundle

osx:QMAKE_CXXFLAGS = -stdlib=libc++

DEFINES += \
    QT_NO_CAST_FROM_ASCII \
    CLANGBACKEND_TESTS \
    DONT_CHECK_COMMAND_COUNTER \
    TESTDATA_DIR=\"R\\\"xxx($$PWD/data)xxx\\\"\"
win32:DEFINES += ECHOSERVER=\"R\\\"xxx($$OUT_PWD/../echo)xxx\\\"\"
unix: DEFINES += ECHOSERVER=\"R\\\"xxx($$OUT_PWD/../echoserver/echo)xxx\\\"\"

SOURCES += \
    activationsequencecontextprocessortest.cpp \
    activationsequenceprocessortest.cpp \
    clangcodecompleteresultstest.cpp \
    clangcompletioncontextanalyzertest.cpp \
    clangipcservertest.cpp \
    clangstringtest.cpp \
    clientserverinprocesstest.cpp \
    clientserveroutsideprocess.cpp \
    codecompletionsextractortest.cpp \
    codecompletiontest.cpp \
    completionchunkstotextconvertertest.cpp \
    createtablesqlstatementbuildertest.cpp \
    lineprefixertest.cpp \
    main.cpp \
    projecttest.cpp \
    readandwritecommandblocktest.cpp \
    spydummy.cpp \
    sqlitecolumntest.cpp \
    sqlitedatabasebackendtest.cpp \
    sqlitedatabasetest.cpp \
    sqlitestatementtest.cpp \
    sqlitetabletest.cpp \
    sqlstatementbuildertest.cpp \
    translationunitstest.cpp \
    translationunittest.cpp \
    unsavedfilestest.cpp \
    utf8test.cpp

HEADERS += \
    gtest-qt-printing.h \
    mockipclient.h \
    mockipcserver.h \
    spydummy.h

OTHER_FILES += $$files(data/*)
