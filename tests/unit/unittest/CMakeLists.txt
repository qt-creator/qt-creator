find_package(Googletest MODULE)
find_package(GoogleBenchmark MODULE)

if (NOT Googletest_FOUND)
  message(STATUS "Googletest was not found. Please set GOOGLETEST_DIR (CMake or Environment) variable.")
  message(STATUS "Have a look at cmake/FindGoogletest.cmake file for more details.")
  message(STATUS "unittest module will be skipped.")
  return()
endif()

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
endif()

add_qtc_test(unittest GTEST
  INCLUDES
    BEFORE "../mockup"
  DEPENDS
    Qt5::Core Qt5::Network Qt5::Widgets
    Qt5::Xml Qt5::Concurrent Qt5::Qml Qt5::Gui
    Googletest
    clangrefactoringbackend_lib clangbackend_lib clangpchmanagerbackend_lib
    CPlusPlus Utils
  DEFINES
    QT_NO_CAST_TO_ASCII
    QT_RESTRICTED_CAST_FROM_ASCII
    UNIT_TESTS
    DONT_CHECK_MESSAGE_COUNTER
    QTC_RESOURCE_DIR="${PROJECT_SOURCE_DIR}/share/qtcreator"
    TESTDATA_DIR="${CMAKE_CURRENT_BINARY_DIR}/data"
    ECHOSERVER="$<TARGET_FILE_DIR:echo>/echo"
    CPPTOOLS_JSON="${CMAKE_CURRENT_BINARY_DIR}/CppTools.json"
  SOURCES
    builddependenciesprovider-test.cpp
    builddependenciesstorage-test.cpp
    clangindexingsettingsmanager-test.cpp
    clangpathwatcher-test.cpp
    clangqueryexamplehighlightmarker-test.cpp
    clangqueryhighlightmarker-test.cpp
    clientserverinprocess-test.cpp
    clientserveroutsideprocess-test.cpp
    commandlinebuilder-test.cpp
    compare-operators.h
    compilationdatabaseutils-test.cpp
    compileroptionsbuilder-test.cpp
    conditionally-disabled-tests.h
    cppprojectfilecategorizer-test.cpp
    cppprojectinfogenerator-test.cpp
    cppprojectpartchooser-test.cpp
    dummyclangipcclient.h
    dynamicastmatcherdiagnosticcontainer-matcher.h
    eventspy.cpp eventspy.h
    fakeprocess.cpp fakeprocess.h
    filepathcache-test.cpp
    filepathstoragesqlitestatementfactory-test.cpp
    filepathstorage-test.cpp
    filepath-test.cpp
    filepathview-test.cpp
    filestatuscache-test.cpp
    filesystem-utilities.h
    generatedfiles-test.cpp
    googletest.h
    google-using-declarations.h
    gtest-creator-printing.cpp gtest-creator-printing.h
    gtest-llvm-printing.h
    gtest-qt-printing.cpp gtest-qt-printing.h
    gtest-std-printing.h
    headerpathfilter-test.cpp
    highlightingresultreporter-test.cpp
    lineprefixer-test.cpp
    locatorfilter-test.cpp
    matchingtext-test.cpp
    mimedatabase-utilities.cpp mimedatabase-utilities.h
    mockbuilddependenciesprovider.h
    mockbuilddependenciesstorage.h
    mockbuilddependencygenerator.h
    mockclangcodemodelclient.h
    mockclangcodemodelserver.h
    mockclangpathwatcher.h
    mockclangpathwatchernotifier.h
    mockcppmodelmanager.h
    mockeditormanager.h
    mockfilepathcaching.h
    mockfilepathstorage.h
    mockfutureinterface.h
    mockgeneratedfiles.h
    mockmodifiedtimechecker.h
    mockmutex.h
    mockpchcreator.h
    mockpchmanagerclient.h
    mockpchmanagernotifier.h
    mockpchmanagerserver.h
    mockpchtaskgenerator.h
    mockpchtaskqueue.h
    mockpchtasksmerger.h
    mockprecompiledheaderstorage.h
    mockprocessor.h
    mockprocessormanager.h
    mockprogressmanager.h
    mockprojectpartprovider.h
    mockprojectpartqueue.h
    mockprojectpartsmanager.h
    mockprojectpartsstorage.h
    mockqfilesystemwatcher.h
    mockqueue.h
    mocksearch.h
    mocksearchhandle.h
    mocksearchresult.h
    mocksqlitedatabase.h
    mocksqlitereadstatement.cpp
    mocksqlitereadstatement.h
    mocksqlitestatement.h
    mocksqlitetransactionbackend.h
    mocksqlitewritestatement.h
    mocksymbolindexertaskqueue.h
    mocksymbolindexing.h
    mocksymbolquery.h
    mocksymbolscollector.h
    mocksymbolstorage.h
    mocksyntaxhighligher.h
    mocktaskscheduler.h
    mocktimer.cpp mocktimer.h
    modifiedtimechecker-test.cpp
    nativefilepath-test.cpp
    nativefilepathview-test.cpp
    pchmanagerclientserverinprocess-test.cpp
    pchmanagerclient-test.cpp
    pchmanagerserver-test.cpp
    pchtaskgenerator-test.cpp
    pchtaskqueue-test.cpp
    pchtasksmerger-test.cpp
    precompiledheaderstorage-test.cpp
    preprocessormacrocollector-test.cpp
    processcreator-test.cpp
    processevents-utilities.cpp processevents-utilities.h
    processormanager-test.cpp
    progresscounter-test.cpp
    projectpartartefact-test.cpp
    projectpartsmanager-test.cpp
    projectpartsstorage-test.cpp
    projectupdater-test.cpp
    readandwritemessageblock-test.cpp
    refactoringdatabaseinitializer-test.cpp
    refactoringprojectupdater-test.cpp
    rundocumentparse-utility.h
    sizedarray-test.cpp
    smallstring-test.cpp
    sourcerangecontainer-matcher.h
    sourcerangefilter-test.cpp
    sourcesmanager-test.cpp
    spydummy.cpp spydummy.h
    sqliteindex-test.cpp
    sqliteteststatement.h
    sqlitetransaction-test.cpp
    stringcache-test.cpp
    symbolindexertaskqueue-test.cpp
    symbolindexer-test.cpp
    symbolquery-test.cpp
    symbolsfindfilter-test.cpp
    symbolstorage-test.cpp
    task.cpp
    taskscheduler-test.cpp
    testenvironment.h
    tokenprocessor-test.cpp
    toolchainargumentscache-test.cpp
    unittests-main.cpp
    unittest-utility-functions.h
    usedmacrofilter-test.cpp
    utf8-test.cpp
    sqlitecolumn-test.cpp
    sqlitedatabasebackend-test.cpp
    sqlitedatabase-test.cpp
    sqlitestatement-test.cpp
    sqlitetable-test.cpp
    sqlstatementbuilder-test.cpp
    createtablesqlstatementbuilder-test.cpp
    sqlitevalue-test.cpp
)

extend_qtc_test(unittest
  DEFINES
    $<TARGET_PROPERTY:Sqlite,INTERFACE_COMPILE_DEFINITIONS>
  SOURCES
    ../../../src/libs/3rdparty/sqlite/sqlite3.c
)

# Do not work on the source directory data
add_custom_command(TARGET unittest POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/data"
          "${CMAKE_CURRENT_BINARY_DIR}/data"
)

# create fake CppTools.json for the mime type definitions
file(READ "${PROJECT_SOURCE_DIR}/src/plugins/cpptools/CppTools.json.in" plugin_json_in)
string(REPLACE "\\\"" "\"" plugin_json_in ${plugin_json_in})
string(REPLACE "\\'" "'" plugin_json_in ${plugin_json_in})
string(REPLACE "$$QTCREATOR_VERSION" "${IDE_VERSION}" plugin_json_in ${plugin_json_in})
string(REPLACE "$$QTCREATOR_COMPAT_VERSION" "${IDE_VERSION_COMPAT}" plugin_json_in ${plugin_json_in})
string(REPLACE "$$QTCREATOR_COPYRIGHT_YEAR" "${IDE_COPYRIGHT_YEAR}" plugin_json_in ${plugin_json_in})
string(REPLACE "$$dependencyList" "\"Dependencies\" : []" plugin_json_in ${plugin_json_in})
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/CppTools.json" ${plugin_json_in}})

extend_qtc_test(unittest
  CONDITION TARGET libclang
  INCLUDES "${CLANG_INCLUDE_DIRS}"
  DEPENDS libclang
  SOURCES
    activationsequencecontextprocessor-test.cpp
    activationsequenceprocessor-test.cpp
    chunksreportedmonitor.cpp
    clangasyncjob-base.cpp
    clangcodecompleteresults-test.cpp
    clangcodemodelserver-test.cpp
    clangcompletecodejob-test.cpp
    clangcompletioncontextanalyzer-test.cpp
    clangdiagnosticfilter-test.cpp
    clangdocumentprocessors-test.cpp
    clangdocumentprocessor-test.cpp
    clangdocuments-test.cpp
    clangdocumentsuspenderresumer-test.cpp
    clangdocument-test.cpp
    clangfixitoperation-test.cpp
    clangfollowsymbol-test.cpp
    clangisdiagnosticrelatedtolocation-test.cpp
    clangjobqueue-test.cpp
    clangjobs-test.cpp
    clangparsesupportivetranslationunitjob-test.cpp
    clangreferencescollector-test.cpp
    clangrequestannotationsjob-test.cpp
    clangrequestreferencesjob-test.cpp
    clangresumedocumentjob-test.cpp
    clangstring-test.cpp
    clangsupportivetranslationunitinitializer-test.cpp
    clangsuspenddocumentjob-test.cpp
    clangtooltipinfo-test.cpp
    clangtranslationunits-test.cpp
    clangtranslationunit-test.cpp
    clangupdateannotationsjob-test.cpp
    codecompleter-test.cpp
    codecompletionsextractor-test.cpp
    completionchunkstotextconverter-test.cpp
    cursor-test.cpp
    diagnosticset-test.cpp
    diagnostic-test.cpp
    fixit-test.cpp
    senddocumenttracker-test.cpp
    skippedsourceranges-test.cpp
    sourcelocation-test.cpp
    sourcerange-test.cpp
    token-test.cpp
    translationunitupdater-test.cpp
    unsavedfiles-test.cpp
    unsavedfile-test.cpp
    utf8positionfromlinecolumn-test.cpp
    chunksreportedmonitor.h
    clangasyncjob-base.h
    clangcompareoperators.h
    diagnosticcontainer-matcher.h
)

extend_qtc_test(unittest
  CONDITION TARGET clangTooling
  DEFINES CLANG_UNIT_TESTS
  DEPENDS clangTooling clangIndex clangQuery
  SOURCES
    gtest-llvm-printing.cpp
    clangquerygatherer-test.cpp
    clangqueryprojectfindfilter-test.cpp
    clangquery-test.cpp
    gtest-clang-printing.cpp gtest-clang-printing.h
    pchcreator-test.cpp
    refactoringclientserverinprocess-test.cpp
    refactoringclient-test.cpp
    refactoringcompilationdatabase-test.cpp
    refactoringengine-test.cpp
    refactoringserver-test.cpp
    sourcerangeextractor-test.cpp
    symbolindexing-test.cpp
    symbolscollector-test.cpp
    testclangtool.cpp testclangtool.h
    usedmacrocollector-test.cpp
    builddependencycollector-test.cpp
    mockrefactoringclient.h
    mockrefactoringserver.h
)

extend_qtc_test(unittest
  CONDITION TARGET clangFormat
  DEPENDS clangFormat
  SOURCES
    clangformat-test.cpp
)

extend_qtc_test(unittest
  CONDITION TARGET GoogleBenchmark
  DEPENDS GoogleBenchmark
  SOURCES
    smallstring-benchmark.cpp
)

finalize_qtc_gtest(unittest ".c$")

# Path needs to be before CppTools
target_include_directories(unittest
  PRIVATE
    BEFORE $<TARGET_PROPERTY:clangrefactoringbackend_lib,INTERFACE_INCLUDE_DIRECTORIES>
    BEFORE ../../../src/plugins/clangrefactoring
    BEFORE ../../../src/plugins
)

get_target_property(ClangSupportSources ClangSupport SOURCES)
get_target_property(ClangSupportSourcesDir ClangSupport SOURCES_DIR)
extend_qtc_test(unittest
  SOURCES_PREFIX "${ClangSupportSourcesDir}"
  SOURCES ${ClangSupportSources}
  DEFINES
    $<TARGET_PROPERTY:ClangSupport,INTERFACE_COMPILE_DEFINITIONS>
    CLANGSUPPORT_BUILD_LIB
  INCLUDES
    $<TARGET_PROPERTY:ClangSupport,INTERFACE_INCLUDE_DIRECTORIES>
)

get_target_property(ClangCodeModelSourcesDir ClangCodeModel SOURCES_DIR)
extend_qtc_test(unittest
  SOURCES_PREFIX "${ClangCodeModelSourcesDir}"
  SOURCES
    clangactivationsequencecontextprocessor.cpp clangactivationsequencecontextprocessor.h
    clangactivationsequenceprocessor.cpp clangactivationsequenceprocessor.h
    clangcompletionchunkstotextconverter.cpp clangcompletionchunkstotextconverter.h
    clangcompletioncontextanalyzer.cpp clangcompletioncontextanalyzer.h
    clangdiagnosticfilter.cpp clangdiagnosticfilter.h
    clangfixitoperation.cpp clangfixitoperation.h
    clanghighlightingresultreporter.cpp clanghighlightingresultreporter.h
    clanguiheaderondiskmanager.cpp clanguiheaderondiskmanager.h
    clangisdiagnosticrelatedtolocation.h
)

get_target_property(CompilationDatabasePMSourcesDir CompilationDatabaseProjectManager SOURCES_DIR)
extend_qtc_test(unittest
  SOURCES_PREFIX "${CompilationDatabasePMSourcesDir}"
  SOURCES
    compilationdatabaseutils.cpp compilationdatabaseutils.h
)

get_target_property(CoreSourcesDir Core SOURCES_DIR)
extend_qtc_test(unittest
  SOURCES_PREFIX "${CoreSourcesDir}"
  DEFINES CORE_STATIC_LIBRARY
  SOURCES
    coreicons.cpp coreicons.h
    id.h
    find/ifindfilter.cpp find/ifindfilter.h
    locator/ilocatorfilter.cpp locator/ilocatorfilter.h
)

get_target_property(CppToolsSourcesDir CppTools SOURCES_DIR)
extend_qtc_test(unittest
  SOURCES_PREFIX "${CppToolsSourcesDir}"
  DEFINES CPPTOOLS_STATIC_LIBRARY
  SOURCES
    cppprojectfile.cpp cppprojectfile.h
    senddocumenttracker.cpp senddocumenttracker.h
    projectpart.cpp projectpart.h
    compileroptionsbuilder.cpp compileroptionsbuilder.h
    cppprojectfilecategorizer.cpp cppprojectfilecategorizer.h
    projectinfo.cpp projectinfo.h
    cppprojectinfogenerator.cpp cppprojectinfogenerator.cpp
    cppprojectpartchooser.cpp cppprojectpartchooser.h
    headerpathfilter.cpp headerpathfilter.h
)

get_target_property(ProjectExplorerSourcesDir ProjectExplorer SOURCES_DIR)
extend_qtc_test(unittest
  SOURCES_PREFIX "${ProjectExplorerSourcesDir}"
  DEFINES PROJECTEXPLORER_STATIC_LIBRARY
  SOURCES
    projectmacro.cpp projectmacro.h
)

extend_qtc_test(unittest
  SOURCES_PREFIX ../../../src/plugins/clangrefactoring
  SOURCES
    clangqueryexamplehighlighter.cpp clangqueryexamplehighlighter.h
    clangqueryexamplehighlightmarker.h
    clangqueryhighlighter.cpp clangqueryhighlighter.h
    clangqueryhighlightmarker.h
    clangqueryprojectsfindfilter.cpp clangqueryprojectsfindfilter.h
    projectpartutilities.cpp projectpartutilities.h
    refactoringclient.cpp refactoringclient.h
    refactoringconnectionclient.cpp refactoringconnectionclient.h
    refactoringengine.cpp refactoringengine.h
    refactoringprojectupdater.cpp refactoringprojectupdater.h
    searchinterface.h
    searchhandle.cpp searchhandle.h
    clangsymbolsfindfilter.cpp clangsymbolsfindfilter.h
    symbolqueryinterface.h
    symbol.h
    projectpartproviderinterface.h
    editormanagerinterface.h
    locatorfilter.cpp locatorfilter.h
)

extend_qtc_test(unittest
  SOURCES_PREFIX ../../../src/plugins/clangpchmanager
  DEFINES CLANGPCHMANAGER_STATIC_LIB
  SOURCES
    clangindexingprojectsettings.cpp clangindexingprojectsettings.h
    clangindexingsettingsmanager.cpp clangindexingsettingsmanager.h
    pchmanagerclient.h pchmanagerclient.cpp
    pchmanagernotifierinterface.h pchmanagernotifierinterface.cpp
    pchmanagerconnectionclient.h pchmanagerconnectionclient.cpp
    clangpchmanager_global.h
    preprocessormacrocollector.cpp preprocessormacrocollector.h
    projectupdater.h projectupdater.cpp
    pchmanagerprojectupdater.h pchmanagerprojectupdater.cpp
    progressmanager.h
    progressmanagerinterface.h
)

get_target_property(ClangFormatSourcesDir ClangFormat SOURCES_DIR)
extend_qtc_test(unittest
  SOURCES_PREFIX "${ClangFormatSourcesDir}"
  DEFINES CLANGPCHMANAGER_STATIC_LIB
  SOURCES
    clangformatconstants.h
    clangformatbaseindenter.cpp clangformatbaseindenter.h
)
