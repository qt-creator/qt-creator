INCLUDEPATH += ../mockup

QT += core network testlib widgets
CONFIG += console c++14 testcase object_parallel_to_source
CONFIG -= app_bundle shared

include(gmock_dependency.pri)
include(clang_dependency.pri)
include(creator_dependency.pri)
include(benchmark_dependency.pri)

OBJECTS_DIR = $$OUT_PWD/obj # workaround for qmake bug in object_parallel_to_source

!msvc:force_debug_info:QMAKE_CXXFLAGS += -fno-omit-frame-pointer

DEFINES += \
    QT_RESTRICTED_CAST_FROM_ASCII \
    UNIT_TESTS \
    DONT_CHECK_MESSAGE_COUNTER \
    TESTDATA_DIR=\"R\\\"xxx($$PWD/data)xxx\\\"\"
msvc: QMAKE_CXXFLAGS_WARN_ON -= -w34100 # 'unreferenced formal parameter' in MATCHER_* functions
win32:DEFINES += ECHOSERVER=\"R\\\"xxx($$OUT_PWD/../echo)xxx\\\"\"
unix: DEFINES += ECHOSERVER=\"R\\\"xxx($$OUT_PWD/../echoserver/echo)xxx\\\"\"

linux {
QMAKE_LFLAGS_RELEASE = #disable optimization
QMAKE_LFLAGS += -fno-merge-debug-strings -fuse-ld=gold
CONFIG(release, debug|release):QMAKE_LFLAGS += -Wl,--strip-debug
}

# create fake CppTools.json for the mime type definitions
dependencyList = "\"Dependencies\" : []"
cpptoolsjson.input = $$PWD/../../../src/plugins/cpptools/CppTools.json.in
cpptoolsjson.output = $$OUT_PWD/CppTools.json
QMAKE_SUBSTITUTES += cpptoolsjson
DEFINES += CPPTOOLS_JSON=\"R\\\"xxx($${cpptoolsjson.output})xxx\\\"\"

SOURCES += \
    clientserverinprocess-test.cpp \
    lineprefixer-test.cpp \
    cppprojectfilecategorizer-test.cpp \
    cppprojectinfogenerator-test.cpp \
    cppprojectpartchooser-test.cpp \
    processevents-utilities.cpp \
    mimedatabase-utilities.cpp \
    readandwritemessageblock-test.cpp \
    sizedarray-test.cpp \
    smallstring-test.cpp \
    spydummy.cpp \
    unittests-main.cpp \
    utf8-test.cpp \
    gtest-qt-printing.cpp \
    gtest-creator-printing.cpp \
    clientserveroutsideprocess-test.cpp \
    clangpathwatcher-test.cpp \
    projectparts-test.cpp \
    stringcache-test.cpp \
    changedfilepathcompressor-test.cpp \
    faketimer.cpp \
    pchgenerator-test.cpp \
    fakeprocess.cpp \
    pchmanagerclient-test.cpp \
    projectupdater-test.cpp \
    pchmanagerserver-test.cpp \
    pchmanagerclientserverinprocess-test.cpp \

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
    clangrequestdocumentannotationsjob-test.cpp \
    clangstring-test.cpp \
    clangtranslationunit-test.cpp \
    clangtranslationunits-test.cpp \
    clangupdatedocumentannotationsjob-test.cpp \
    codecompletionsextractor-test.cpp \
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
    utf8positionfromlinecolumn-test.cpp \
    clangparsesupportivetranslationunitjob-test.cpp \
    clangreparsesupportivetranslationunitjob-test.cpp \
    clangsupportivetranslationunitinitializer-test.cpp \
    codecompleter-test.cpp
}

!isEmpty(LIBTOOLING_LIBS) {
SOURCES += \
    clangquery-test.cpp \
    clangqueryprojectfindfilter-test.cpp \
    refactoringclientserverinprocess-test.cpp \
    refactoringclient-test.cpp \
    refactoringcompilationdatabase-test.cpp \
    refactoringengine-test.cpp \
    refactoringserver-test.cpp \
    symbolfinder-test.cpp \
    sourcerangeextractor-test.cpp \
    gtest-clang-printing.cpp \
    testclangtool.cpp \
    includecollector-test.cpp \
    pchcreator-test.cpp
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
    mimedatabase-utilities.h \
    spydummy.h \
    sourcerangecontainer-matcher.h \
    dynamicastmatcherdiagnosticcontainer-matcher.h \
    mocksearchresult.h \
    mocksearch.h \
    mocksearchhandle.h \
    compare-operators.h \
    gtest-creator-printing.h \
    conditionally-disabled-tests.h \
    mockqfilesystemwatcher.h \
    mockclangpathwatcher.h \
    mockprojectparts.h \
    mockclangpathwatchernotifier.h \
    mockchangedfilepathcompressor.h \
    faketimer.h \
    mockpchgeneratornotifier.h \
    fakeprocess.h \
    mockpchmanagerserver.h \
    mockpchmanagerclient.h \
    testenvironment.h \
    mockpchmanagernotifier.h \
    mockpchcreator.h \
    dummyclangipcclient.h \
    mockclangcodemodelclient.h \
    mockclangcodemodelserver.h

!isEmpty(LIBCLANG_LIBS) {
HEADERS += \
    chunksreportedmonitor.h \
    clangasyncjob-base.h \
    diagnosticcontainer-matcher.h \
    clangcompareoperators.h
}

!isEmpty(LIBTOOLING_LIBS) {
HEADERS += \
    mockrefactoringclientcallback.h \
    mockrefactoringclient.h \
    mockrefactoringserver.h \
    gtest-clang-printing.h \
    testclangtool.h
}

OTHER_FILES += $$files(data/*)
