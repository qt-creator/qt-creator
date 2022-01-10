INCLUDEPATH += ../mockup
INCLUDEPATH += ../mockup/qmldesigner/designercore/include

QT += core network testlib widgets
CONFIG += console c++17 testcase
CONFIG -= app_bundle shared

QTC_UNITTEST_BUILD_CPP_PARSER = $$(QTC_UNITTEST_BUILD_CPP_PARSER)

include(gmock_dependency.pri)
include(clang_dependency.pri)
include(creator_dependency.pri)
include(benchmark_dependency.pri)

requires(isEmpty(QTC_CLANG_BUILDMODE_MISMATCH))

!msvc:force_debug_info:QMAKE_CXXFLAGS += -fno-omit-frame-pointer

DEFINES += \
    QT_NO_CAST_TO_ASCII \
    QT_RESTRICTED_CAST_FROM_ASCII \
    QT_USE_QSTRINGBUILDER \
    UNIT_TESTS \
    DONT_CHECK_MESSAGE_COUNTER \
    QTC_RESOURCE_DIR=\"R\\\"xxx($$PWD/../../../share/qtcreator)xxx\\\"\" \
    TESTDATA_DIR=\"R\\\"xxx($$PWD/data)xxx\\\"\"
msvc: QMAKE_CXXFLAGS_WARN_ON -= -w34100 # 'unreferenced formal parameter' in MATCHER_* functions
win32:DEFINES += ECHOSERVER=\"R\\\"xxx($$OUT_PWD/../echo)xxx\\\"\"
unix: DEFINES += ECHOSERVER=\"R\\\"xxx($$OUT_PWD/../echoserver/echo)xxx\\\"\"

RELATIVE_DATA_PATH = ../../../share/qtcreator
DEFINES += $$shell_quote(RELATIVE_DATA_PATH=\"$$RELATIVE_DATA_PATH\")

win32 {
    RELATIVE_LIBEXEC_PATH = .
} else: macos {
    RELATIVE_LIBEXEC_PATH = ../Resources/libexec
} else {
    RELATIVE_LIBEXEC_PATH = ../libexec/qtcreator
}
DEFINES += $$shell_quote(RELATIVE_LIBEXEC_PATH=\"$$RELATIVE_LIBEXEC_PATH\")

linux {
QMAKE_LFLAGS_RELEASE = #disable optimization
QMAKE_LFLAGS += -fno-merge-debug-strings -fuse-ld=gold
CONFIG(release, debug|release):QMAKE_LFLAGS += -Wl,--strip-debug
}

CONFIG(debug, debug|release): DEFINES += SQLITE_DEBUG


gcc:!clang: QMAKE_CXXFLAGS += -Wno-noexcept-type
msvc{
QMAKE_CXXFLAGS += /bigobj /wd4267 /wd4141 /wd4146 /wd4624
QMAKE_LFLAGS += /INCREMENTAL

}

TEST_RELATIVE_LIBEXEC_PATH = $$relative_path($$IDE_LIBEXEC_PATH, $$OUT_PWD)
win32:TEST_RELATIVE_LIBEXEC_PATH=../$$TEST_RELATIVE_LIBEXEC_PATH
DEFINES += 'TEST_RELATIVE_LIBEXEC_PATH="\\\"$$TEST_RELATIVE_LIBEXEC_PATH\\\""'

SOURCES += \
    clientserverinprocess-test.cpp \
    clientserveroutsideprocess-test.cpp \
    fakeprocess.cpp \
    gtest-creator-printing.cpp \
    gtest-qt-printing.cpp \
    asynchronousimagecache-test.cpp \
    nodelistproperty-test.cpp \
    projectstoragesqlitefunctionregistry-test.cpp \
    storagecache-test.cpp \
    sqlitealgorithms-test.cpp \
    synchronousimagecache-test.cpp \
    imagecachegenerator-test.cpp \
    imagecachestorage-test.cpp \
    lastchangedrowid-test.cpp \
    lineprefixer-test.cpp \
    listmodeleditor-test.cpp \
    processevents-utilities.cpp \
    readandwritemessageblock-test.cpp \
    sizedarray-test.cpp \
    smallstring-test.cpp \
    spydummy.cpp \
    sqlitesessions-test.cpp \
    sqlitevalue-test.cpp \
    eventspy.cpp \
    unittests-main.cpp \
    utf8-test.cpp \
    sqliteindex-test.cpp \
    sqlitetransaction-test.cpp \
    processcreator-test.cpp \
    mocktimer.cpp \
    sqlitecolumn-test.cpp \
    sqlitedatabasebackend-test.cpp \
    sqlitedatabase-test.cpp \
    sqlitestatement-test.cpp \
    sqlitetable-test.cpp \
    sqlstatementbuilder-test.cpp \
    createtablesqlstatementbuilder-test.cpp \
    sqlitereadstatementmock.cpp \
    sqlitewritestatementmock.cpp \
    sqlitereadwritestatementmock.cpp \
    sourcepath-test.cpp \
    sourcepathview-test.cpp \
    projectstorage-test.cpp \
    sourcepathcache-test.cpp

!isEmpty(QTC_UNITTEST_BUILD_CPP_PARSER):SOURCES += matchingtext-test.cpp

!isEmpty(LIBCLANG_LIBS) {
SOURCES += \
    chunksreportedmonitor.cpp \
    clangasyncjob-base.cpp \
    clangcodecompleteresults-test.cpp \
    clangcodemodelserver-test.cpp \
    clangcompletecodejob-test.cpp \
    clangdiagnosticfilter-test.cpp \
    clangdocumentprocessors-test.cpp \
    clangdocumentprocessor-test.cpp \
    clangdocuments-test.cpp \
    clangdocument-test.cpp \
    clangfixitoperation-test.cpp \
    clangfollowsymbol-test.cpp \
    clangisdiagnosticrelatedtolocation-test.cpp \
    clangjobqueue-test.cpp \
    clangjobs-test.cpp \
    clangparsesupportivetranslationunitjob-test.cpp \
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
    cursor-test.cpp \
    diagnosticset-test.cpp \
    diagnostic-test.cpp \
    fixit-test.cpp \
    gtest-clang-printing.cpp \
    highlightingresultreporter-test.cpp \
    skippedsourceranges-test.cpp \
    sourcelocation-test.cpp \
    sourcerange-test.cpp \
    token-test.cpp \
    tokenprocessor-test.cpp \
    translationunitupdater-test.cpp \
    unsavedfiles-test.cpp \
    unsavedfile-test.cpp \
    utf8positionfromlinecolumn-test.cpp \
    clangreferencescollector-test.cpp \
    clangdocumentsuspenderresumer-test.cpp \
    readexporteddiagnostics-test.cpp

!isEmpty(QTC_UNITTEST_BUILD_CPP_PARSER):SOURCE += \
    clangcompletioncontextanalyzer-test.cpp \
    activationsequencecontextprocessor-test.cpp \
    activationsequenceprocessor-test.cpp

}

!isEmpty(LIBTOOLING_LIBS) {
SOURCES += \
    gtest-llvm-printing.cpp \
}

!isEmpty(CLANGFORMAT_LIBS) {
    SOURCES += clangformat-test.cpp
}

!isEmpty(GOOGLEBENCHMARK_DIR):exists($$GOOGLEBENCHMARK_DIR) {
SOURCES += \
    smallstring-benchmark.cpp
}

HEADERS += \
    abstractviewmock.h \
    compare-operators.h \
    conditionally-disabled-tests.h \
    dummyclangipcclient.h \
    dynamicastmatcherdiagnosticcontainer-matcher.h \
    eventspy.h \
    fakeprocess.h \
    filesystem-utilities.h \
    googletest.h \
    gtest-creator-printing.h \
    gtest-llvm-printing.h \
    gtest-qt-printing.h \
    gtest-std-printing.h \
    imagecachecollectormock.h \
    mockclangcodemodelclient.h \
    mockclangcodemodelserver.h \
    mockimagecachegenerator.h \
    mockimagecachestorage.h \
    mocklistmodeleditorview.h \
    mockqfilesystemwatcher.h \
    mocksyntaxhighligher.h \
    mocktimestampprovider.h \
    notification.h \
    processevents-utilities.h \
    sourcerangecontainer-matcher.h \
    spydummy.h \
    google-using-declarations.h \
    sqliteteststatement.h \
    mockmutex.h \
    mocksqlitestatement.h \
    unittest-utility-functions.h \
    rundocumentparse-utility.h \
    mocktimer.h \
    mocksqlitetransactionbackend.h \
    mockqueue.h \
    mockfutureinterface.h \
    ../mockup/qmldesigner/designercore/include/nodeinstanceview.h \
    ../mockup/qmldesigner/designercore/include/rewriterview.h \
    ../mockup/qmldesigner/designercore/include/itemlibraryitem.h\
    sqlitedatabasemock.h \
    sqlitereadstatementmock.h \
    sqlitestatementmock.h \
    sqlitetransactionbackendmock.h \
    sqlitewritestatementmock.h \
    sqlitereadwritestatementmock.h \
    projectstoragemock.h


!isEmpty(LIBCLANG_LIBS) {
HEADERS += \
    chunksreportedmonitor.h \
    clangasyncjob-base.h \
    clangcompareoperators.h \
    diagnosticcontainer-matcher.h \
    gtest-clang-printing.h
}

OTHER_FILES += $$files(data/*) $$files(data/include/*)
