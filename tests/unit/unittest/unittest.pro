INCLUDEPATH += ../mockup

include(gmock_dependency.pri)
include(clang_dependency.pri)
include(creator_dependency.pri)
include(benchmark_dependency.pri)

QT += core network testlib widgets
CONFIG += console c++14 testcase object_parallel_to_source
CONFIG -= app_bundle

OBJECTS_DIR = $$OUT_PWD/obj # workaround for qmake bug in object_parallel_to_source

osx:QMAKE_CXXFLAGS = -stdlib=libc++

force_debug_info:QMAKE_CXXFLAGS += -fno-omit-frame-pointer

DEFINES += \
    QT_RESTRICTED_CAST_FROM_ASCII \
    UNIT_TESTS \
    DONT_CHECK_MESSAGE_COUNTER \
    TESTDATA_DIR=\"R\\\"xxx($$PWD/data)xxx\\\"\"
msvc: QMAKE_CXXFLAGS_WARN_ON -= -w34100 # 'unreferenced formal parameter' in MATCHER_* functions
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
    skippedsourcerangestest.cpp \
    highlightingmarksreportertest.cpp \
    chunksreportedmonitor.cpp \
    unsavedfiletest.cpp \
    clangisdiagnosticrelatedtolocationtest.cpp \
    smallstringtest.cpp \
    highlightingmarkstest.cpp \
    sizedarraytest.cpp \
    utf8positionfromlinecolumntest.cpp \
    translationunitupdatertest.cpp \
    testutils.cpp \
    clangasyncjobtest.cpp \
    clangcompletecodejobtest.cpp \
    clangcreateinitialdocumentpreamblejobtest.cpp \
    clangjobqueuetest.cpp \
    refactoringcompilationdatabasetest.cpp \
    symbolfindertest.cpp \
    refactoringclientserverinprocesstest.cpp \
    refactoringservertest.cpp \
    refactoringenginetest.cpp \
    refactoringclienttest.cpp \
    clangjobstest.cpp \
    clangrequestdocumentannotationsjobtest.cpp \
    clangupdatedocumentannotationsjobtest.cpp

exists($$GOOGLEBENCHMARK_DIR) {
SOURCES += \
    smallstringbenchmark.cpp
}

HEADERS += \
    gtest-qt-printing.h \
    mockclangcodemodelclient.h \
    mockclangcodemodelserver.h \
    mockrefactoringclient.h \
    mockrefactoringserver.h \
    spydummy.h \
    dummyclangipcclient.h \
    matcher-diagnosticcontainer.h \
    chunksreportedmonitor.h \
    testutils.h \
    clangasyncjobtest.h \
    refactoringclientcallbackmock.h \
    filesystemutilities.h

OTHER_FILES += $$files(data/*)
