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
    QT_NO_CAST_TO_ASCII \
    QT_RESTRICTED_CAST_FROM_ASCII \
    QT_USE_FAST_OPERATOR_PLUS \
    QT_USE_FAST_CONCATENATION \
    UNIT_TESTS \
    DONT_CHECK_MESSAGE_COUNTER \
    QTC_RESOURCE_DIR=\"R\\\"xxx($$PWD/../../../share/qtcreator)xxx\\\"\" \
    TESTDATA_DIR=\"R\\\"xxx($$PWD/data)xxx\\\"\"
msvc: QMAKE_CXXFLAGS_WARN_ON -= -w34100 # 'unreferenced formal parameter' in MATCHER_* functions
win32:DEFINES += ECHOSERVER=\"R\\\"xxx($$OUT_PWD/../echo)xxx\\\"\"
unix: DEFINES += ECHOSERVER=\"R\\\"xxx($$OUT_PWD/../echoserver/echo)xxx\\\"\"

linux {
QMAKE_LFLAGS_RELEASE = #disable optimization
QMAKE_LFLAGS += -fno-merge-debug-strings -fuse-ld=gold
CONFIG(release, debug|release):QMAKE_LFLAGS += -Wl,--strip-debug
}

gcc:!clang: QMAKE_CXXFLAGS += -Wno-noexcept-type

# create fake CppTools.json for the mime type definitions
dependencyList = "\"Dependencies\" : []"
cpptoolsjson.input = $$PWD/../../../src/plugins/cpptools/CppTools.json.in
cpptoolsjson.output = $$OUT_PWD/CppTools.json
QMAKE_SUBSTITUTES += cpptoolsjson
DEFINES += CPPTOOLS_JSON=\"R\\\"xxx($${cpptoolsjson.output})xxx\\\"\"

SOURCES += \
    changedfilepathcompressor-test.cpp \
    clangpathwatcher-test.cpp \
    clangqueryexamplehighlightmarker-test.cpp \
    clangqueryhighlightmarker-test.cpp \
    clientserverinprocess-test.cpp \
    clientserveroutsideprocess-test.cpp \
    cppprojectfilecategorizer-test.cpp \
    cppprojectinfogenerator-test.cpp \
    cppprojectpartchooser-test.cpp \
    fakeprocess.cpp \
    filepath-test.cpp \
    filepathview-test.cpp \
    gtest-creator-printing.cpp \
    gtest-qt-printing.cpp \
    lineprefixer-test.cpp \
    locatorfilter-test.cpp \
    matchingtext-test.cpp \
    mimedatabase-utilities.cpp \
    pchmanagerclientserverinprocess-test.cpp \
    pchmanagerclient-test.cpp \
    pchmanagerserver-test.cpp \
    processevents-utilities.cpp \
    projectparts-test.cpp \
    projectupdater-test.cpp \
    readandwritemessageblock-test.cpp \
    sizedarray-test.cpp \
    smallstring-test.cpp \
    sourcerangefilter-test.cpp \
    spydummy.cpp \
    symbolindexer-test.cpp \
    symbolsfindfilter-test.cpp \
    stringcache-test.cpp \
    eventspy.cpp \
    unittests-main.cpp \
    utf8-test.cpp \
    symbolstorage-test.cpp \
    mocksqlitereadstatement.cpp \
    symbolquery-test.cpp \
    sqliteindex-test.cpp \
    sqlitetransaction-test.cpp \
    refactoringdatabaseinitializer-test.cpp \
    filepathcache-test.cpp \
    filepathstorage-test.cpp \
    filepathstoragesqlitestatementfactory-test.cpp \
    processcreator-test.cpp \
    nativefilepath-test.cpp \
    nativefilepathview-test.cpp \
    mocktimer.cpp \
    tokenprocessor-test.cpp \
    projectpartartefact-test.cpp \
    filestatuscache-test.cpp \
    highlightingresultreporter-test.cpp \
    precompiledheaderstorage-test.cpp \
    generatedfiles-test.cpp \
    sourcesmanager-test.cpp \
    symbolindexertaskqueue-test.cpp \
    refactoringprojectupdater-test.cpp \
    projectpartqueue-test.cpp \
    processormanager-test.cpp \
    taskscheduler-test.cpp \
    compileroptionsbuilder-test.cpp \
    pchtaskgenerator-test.cpp \
    compilationdatabaseutils-test.cpp \
    builddependenciesprovider-test.cpp \
    builddependenciesstorage-test.cpp

!isEmpty(LIBCLANG_LIBS) {
SOURCES += \
    activationsequencecontextprocessor-test.cpp \
    activationsequenceprocessor-test.cpp \
    chunksreportedmonitor.cpp \
    clangasyncjob-base.cpp \
    clangcodecompleteresults-test.cpp \
    clangcodemodelserver-test.cpp \
    clangcompletecodejob-test.cpp \
    clangcompletioncontextanalyzer-test.cpp \
    clangdiagnosticfilter-test.cpp \
    clangdocumentprocessors-test.cpp \
    clangdocumentprocessor-test.cpp \
    clangdocuments-test.cpp \
    clangdocumentsuspenderresumer-test.cpp \
    clangdocument-test.cpp \
    clangfixitoperation-test.cpp \
    clangfollowsymbol-test.cpp \
    clangisdiagnosticrelatedtolocation-test.cpp \
    clangjobqueue-test.cpp \
    clangjobs-test.cpp \
    clangparsesupportivetranslationunitjob-test.cpp \
    clangreferencescollector-test.cpp \
    clangrequestannotationsjob-test.cpp \
    clangrequestreferencesjob-test.cpp \
    clangresumedocumentjob-test.cpp \
    clangstring-test.cpp \
    clangsupportivetranslationunitinitializer-test.cpp \
    clangsuspenddocumentjob-test.cpp \
    clangtooltipinfo-test.cpp \
    clangtranslationunits-test.cpp \
    clangtranslationunit-test.cpp \
    clangupdateannotationsjob-test.cpp \
    codecompleter-test.cpp \
    codecompletionsextractor-test.cpp \
    completionchunkstotextconverter-test.cpp \
    createtablesqlstatementbuilder-test.cpp \
    cursor-test.cpp \
    diagnosticset-test.cpp \
    diagnostic-test.cpp \
    fixit-test.cpp \
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
    token-test.cpp \
    translationunitupdater-test.cpp \
    unsavedfiles-test.cpp \
    unsavedfile-test.cpp \
    utf8positionfromlinecolumn-test.cpp \
}

!isEmpty(LIBTOOLING_LIBS) {
SOURCES += \
    clangquerygatherer-test.cpp \
    clangqueryprojectfindfilter-test.cpp \
    clangquery-test.cpp \
    gtest-clang-printing.cpp \
    pchcreator-test.cpp \
    refactoringclientserverinprocess-test.cpp \
    refactoringclient-test.cpp \
    refactoringcompilationdatabase-test.cpp \
    refactoringengine-test.cpp \
    refactoringserver-test.cpp \
    sourcerangeextractor-test.cpp \
    symbolindexing-test.cpp \
    symbolscollector-test.cpp \
    symbolfinder-test.cpp \
    testclangtool.cpp \
    usedmacrocollector-test.cpp \
    builddependencycollector-test.cpp
}

exists($$GOOGLEBENCHMARK_DIR) {
SOURCES += \
    smallstring-benchmark.cpp
}

HEADERS += \
    compare-operators.h \
    conditionally-disabled-tests.h \
    dummyclangipcclient.h \
    dynamicastmatcherdiagnosticcontainer-matcher.h \
    eventspy.h \
    fakeprocess.h \
    filesystem-utilities.h \
    googletest.h \
    gtest-creator-printing.h \
    gtest-qt-printing.h \
    mimedatabase-utilities.h \
    mockclangcodemodelclient.h \
    mockclangcodemodelserver.h \
    mockclangpathwatcher.h \
    mockclangpathwatchernotifier.h \
    mockpchcreator.h \
    mockpchmanagerclient.h \
    mockpchmanagernotifier.h \
    mockpchmanagerserver.h \
    mockprojectparts.h \
    mockqfilesystemwatcher.h \
    mocksearch.h \
    mocksearchhandle.h \
    mocksearchresult.h \
    mocksyntaxhighligher.h \
    processevents-utilities.h \
    sourcerangecontainer-matcher.h \
    spydummy.h \
    testenvironment.h \
    mocksymbolscollector.h \
    mocksymbolstorage.h \
    mocksqlitewritestatement.h \
    mocksqlitedatabase.h \
    mocksqlitereadstatement.h \
    google-using-declarations.h \
    mocksymbolindexing.h \
    sqliteteststatement.h \
    mockmutex.h \
    mockfilepathstorage.h \
    mockfilepathcaching.h \
    mocksqlitestatement.h \
    unittest-utility-functions.h \
    mocksymbolquery.h \
    rundocumentparse-utility.h \
    mocktimer.h \
    mocksqlitetransactionbackend.h \
    mockprojectpartprovider.h \
    mockprecompiledheaderstorage.h \
    mockeditormanager.h \
    mocksymbolindexertaskqueue.h \
    mockcppmodelmanager.h \
    mockgeneratedfiles.h \
    mockqueue.h \
    mockprojectpartqueue.h \
    mockprocessor.h \
    mockprocessormanager.h \
    mocktaskscheduler.h \
    mockbuilddependenciesprovider.h \
    mockmodifiedtimechecker.h \
    mockbuilddependenciesstorage.h \
    mockbuilddependencygenerator.h

!isEmpty(LIBCLANG_LIBS) {
HEADERS += \
    chunksreportedmonitor.h \
    clangasyncjob-base.h \
    clangcompareoperators.h \
    diagnosticcontainer-matcher.h \
}

!isEmpty(LIBTOOLING_LIBS) {
HEADERS += \
    gtest-clang-printing.h \
    mockrefactoringclient.h \
    mockrefactoringserver.h \
    testclangtool.h \
}

OTHER_FILES += $$files(data/*) $$files(data/include/*)
