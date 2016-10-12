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
    clientserverinprocess-test.cpp \
    clientserveroutsideprocess.cpp \
    lineprefixer-test.cpp \
    processevents-utilities.cpp \
    readandwritemessageblock-test.cpp \
    sizedarray-test.cpp \
    smallstring-test.cpp \
    spydummy.cpp \
    unittests-main.cpp \
    utf8-test.cpp

!isEmpty(LIBCLANG_LIBS) {
SOURCES += \
    activationsequencecontextprocessor-test.cpp \
    activationsequenceprocessor-test.cpp \
    chunksreportedmonitor.cpp \
    clangasyncjob-base.cpp \
    clangcodecompleteresults-test.cpp \
    clangcompletecodejob-test.cpp \
    clangcompletioncontextanalyzer-test.cpp \
    clangcreateinitialdocumentpreamblejob-test.cpp \
    clangdiagnosticfilter-test.cpp \
    clangdocuments-test.cpp \
    clangdocument-test.cpp \
    clangdocumentprocessor-test.cpp \
    clangdocumentprocessors-test.cpp \
    clangfixitoperation-test.cpp \
    clangipcserver-test.cpp \
    clangisdiagnosticrelatedtolocation-test.cpp \
    clangjobqueue-test.cpp \
    clangjobs-test.cpp \
    clangparsesupportivetranslationunitjobtest.cpp \
    clangreparsesupportivetranslationunitjobtest.cpp \
    clangrequestdocumentannotationsjob-test.cpp \
    clangsupportivetranslationunitinitializertest.cpp \
    clangstring-test.cpp \
    clangtranslationunit-test.cpp \
    clangtranslationunits-test.cpp \
    clangupdatedocumentannotationsjob-test.cpp \
    codecompletionsextractor-test.cpp \
    codecompletion-test.cpp \
    completionchunkstotextconverter-test.cpp \
    createtablesqlstatementbuilder-test.cpp \
    cursor-test.cpp \
    diagnosticset-test.cpp \
    diagnostic-test.cpp \
    fixit-test.cpp \
    highlightingmarksreporter-test.cpp \
    highlightingmarks-test.cpp \
    projectpart-test.cpp \
    senddocumenttracker-test.cpp \
    skippedsourceranges-test.cpp \
    sourcelocation-test.cpp \
    sourcerange-test.cpp \
    sqlitecolumn-test.cpp \
    sqlitedatabasebackend-test.cpp \
    sqlitedatabase-test.cpp \
    sqlitestatement-test.cpp \
    sqlitetable-test.cpp \
    sqlstatementbuilder-test.cpp \
    translationunitupdater-test.cpp \
    unsavedfiles-test.cpp \
    unsavedfile-test.cpp \
    utf8positionfromlinecolumn-test.cpp
}

!isEmpty(LIBTOOLING_LIBS) {
SOURCES += \
    refactoringclientserverinprocess-test.cpp \
    refactoringclient-test.cpp \
    refactoringcompilationdatabase-test.cpp \
    refactoringengine-test.cpp \
    refactoringserver-test.cpp \
    symbolfinder-test.cpp
}

exists($$GOOGLEBENCHMARK_DIR) {
SOURCES += \
    smallstring-benchmark.cpp
}

HEADERS += \
    filesystem-utilities.h \
    googletest.h \
    gtest-qt-printing.h \
    processevents-utilities.h \
    spydummy.h

!isEmpty(LIBCLANG_LIBS) {
HEADERS += \
    chunksreportedmonitor.h \
    clangasyncjob-base.h \
    diagnosticcontainer-matcher.h \
    clangcompareoperators.h \
    dummyclangipcclient.h \
    mockclangcodemodelclient.h \
    mockclangcodemodelserver.h
}

!isEmpty(LIBTOOLING_LIBS) {
HEADERS += \
    mockrefactoringclientcallback.h \
    mockrefactoringclient.h \
    mockrefactoringserver.h
}

OTHER_FILES += $$files(data/*)
