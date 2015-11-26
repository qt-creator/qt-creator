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
    DONT_CHECK_MESSAGE_COUNTER \
    TESTDATA_DIR=\"R\\\"xxx($$PWD/data)xxx\\\"\"
win32-msvc*:QMAKE_CXXFLAGS_WARN_ON -= -w34100 # 'unreferenced formal parameter' in MATCHER_* functions
win32:DEFINES += ECHOSERVER=\"R\\\"xxx($$OUT_PWD/../echo)xxx\\\"\"
unix: DEFINES += ECHOSERVER=\"R\\\"xxx($$OUT_PWD/../echoserver/echo)xxx\\\"\"

SOURCES += \
    activationsequencecontextprocessortest.cpp \
    activationsequenceprocessortest.cpp \
    clangcodecompleteresultstest.cpp \
    clangcompletioncontextanalyzertest.cpp \
    clangdiagnosticfiltertest.cpp \
    clangfixitoperationtest.cpp \
    clangipcservertest.cpp \
    clangstringtest.cpp \
    clientserverinprocesstest.cpp \
    clientserveroutsideprocess.cpp \
    codecompletionsextractortest.cpp \
    codecompletiontest.cpp \
    completionchunkstotextconvertertest.cpp \
    createtablesqlstatementbuildertest.cpp \
    diagnosticsettest.cpp \
    diagnostictest.cpp \
    fixittest.cpp \
    lineprefixertest.cpp \
    main.cpp \
    projecttest.cpp \
    readandwritemessageblocktest.cpp \
    sourcelocationtest.cpp \
    sourcerangetest.cpp \
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
    utf8test.cpp \
    senddocumenttrackertest.cpp \
    cursortest.cpp \
    highlightinginformationstest.cpp \
    skippedsourcerangestest.cpp \
    highlightingmarksreportertest.cpp \
    chunksreportedmonitor.cpp

HEADERS += \
    gtest-qt-printing.h \
    mockipclient.h \
    mockipcserver.h \
    spydummy.h \
    matcher-diagnosticcontainer.h \
    chunksreportedmonitor.h \
    mocksendeditorupdatescallback.h

OTHER_FILES += $$files(data/*)
