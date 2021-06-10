if (NOT QTC_CLANG_BUILDMODE_MATCH)
    message(STATUS "Skipping unittest due to build mode mismatch")
    return()
endif()

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
    BEFORE "../mockup/qmldesigner/designercore/include"
  DEPENDS
    Qt5::Core Qt5::Network Qt5::Widgets
    Qt5::Xml Qt5::Concurrent Qt5::Qml Qt5::Gui
    Qt6Core5Compat
    Googletest
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
    abstractviewmock.h
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
    mocksqlitestatement.h
    mocksqlitetransactionbackend.h
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
    nodelistproperty-test.cpp
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
    sqlitealgorithms-test.cpp
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
    asynchronousimagecache-test.cpp
    synchronousimagecache-test.cpp
    imagecachegenerator-test.cpp
    imagecachestorage-test.cpp
    sqlitedatabasemock.h
    sqlitereadstatementmock.cpp sqlitereadstatementmock.h
    sqlitestatementmock.h
    sqlitetransactionbackendmock.h
    sqlitewritestatementmock.cpp sqlitewritestatementmock.h
    notification.h
    mocktimestampprovider.h
    imagecachecollectormock.h
    mockimagecachegenerator.h
    mockimagecachestorage.h
)

if (NOT TARGET unittest)
    return()
endif()

function(extend_qtc_test_with_target_sources target)
  cmake_parse_arguments(_arg "" "" "DEFINES;INCLUDES" ${ARGN})

  get_target_property(${target}Sources ${target} SOURCES)
  # work around issue with CMake < 3.14 where target sources can contain
  # $<TARGET_OBJECTS:...>
  list(FILTER ${target}Sources EXCLUDE REGEX "^\\$<TARGET_OBJECTS:.*")

  get_target_property(${target}SourcesDir ${target} SOURCES_DIR)
  extend_qtc_test(unittest
    SOURCES_PREFIX "${${target}SourcesDir}"
    SOURCES ${${target}Sources}
    DEFINES
      $<TARGET_PROPERTY:${target},INTERFACE_COMPILE_DEFINITIONS>
      ${_arg_DEFINES}
    INCLUDES
      $<TARGET_PROPERTY:${target},INTERFACE_INCLUDE_DIRECTORIES>
      ${_arg_INCLUDES}
  )
endfunction()

foreach(clang_tools_library clangrefactoringbackend_lib clangbackend_lib clangpchmanagerbackend_lib)
  extend_qtc_test_with_target_sources(${clang_tools_library}
    DEFINES
      CLANGSUPPORT_BUILD_LIB
    INCLUDES
      $<TARGET_PROPERTY:ClangSupport,INTERFACE_INCLUDE_DIRECTORIES>
   )
endforeach()

extend_qtc_test_with_target_sources(Sqlite)

extend_qtc_test(unittest DEFINES _HAVE_SQLITE_CONFIG_H)

if (APPLE)
extend_qtc_test(unittest DEFINES _BSD_SOURCE)
elseif (UNIX)
extend_qtc_test(unittest DEFINES _POSIX_C_SOURCE=200809L _GNU_SOURCE _DEFAULT_SOURCE)
endif()

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
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/CppTools.json" ${plugin_json_in})

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
  CONDITION TARGET ClangFormat
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

set(QmlDesignerDir ../../../src/plugins/qmldesigner)
extend_qtc_test(unittest
  INCLUDES
    "${QmlDesignerDir}"
    "${QmlDesignerDir}/designercore"
    "${QmlDesignerDir}/designercore/include"
    "${QmlDesignerDir}/designercore/imagecache"
    "${QmlDesignerDir}/../../../share/qtcreator/qml/qmlpuppet/interfaces"
    "${QmlDesignerDir}/../../../share/qtcreator/qml/qmlpuppet/types"
  DEFINES
    QMLDESIGNER_TEST DESIGNER_STATIC_CORE_LIBRARY
  SOURCES_PREFIX
    "${QmlDesignerDir}"
  SOURCES
    components/listmodeleditor/listmodeleditormodel.cpp components/listmodeleditor/listmodeleditormodel.h
)
extend_qtc_test(unittest
  SOURCES_PREFIX "${QmlDesignerDir}/designercore"
  SOURCES
    exceptions/exception.cpp
    exceptions/invalidargumentexception.cpp
    exceptions/invalididexception.cpp
    exceptions/invalidmetainfoexception.cpp
    exceptions/invalidmodelnodeexception.cpp
    exceptions/invalidmodelstateexception.cpp
    exceptions/invalidpropertyexception.cpp
    exceptions/invalidqmlsourceexception.cpp
    exceptions/invalidreparentingexception.cpp
    exceptions/invalidslideindexexception.cpp
    exceptions/notimplementedexception.cpp
    exceptions/removebasestateexception.cpp
    exceptions/rewritingexception.cpp

    model/abstractproperty.cpp include/abstractproperty.h
    model/abstractview.cpp include/abstractview.h
    model/annotation.cpp
    model/bindingproperty.cpp include/bindingproperty.h
    model/import.cpp include/import.h
    model/internalproperty.cpp model/internalproperty.h
    model/internalbindingproperty.cpp model/internalbindingproperty.h
    model/internalnodeabstractproperty.cpp model/internalnodeabstractproperty.h
    model/internalnodelistproperty.cpp model/internalnodelistproperty.h
    model/internalnodeproperty.cpp model/internalnodeproperty.h
    model/internalsignalhandlerproperty.cpp model/internalsignalhandlerproperty.h
    model/internalnode.cpp model/internalnode_p.h
    model/internalvariantproperty.cpp model/internalvariantproperty.h
    model/model.cpp include/model.h model/model_p.h
    model/modelnode.cpp include/modelnode.h
    model/nodeabstractproperty.cpp include/nodeabstractproperty.h
    model/nodelistproperty.cpp include/nodelistproperty.h
    model/nodeproperty.cpp include/nodeproperty.h
    model/signalhandlerproperty.cpp include/signalhandlerproperty.h
    model/variantproperty.cpp include/variantproperty.h
    rewritertransaction.cpp rewritertransaction.h
    include/imagecacheauxiliarydata.h
    imagecache/synchronousimagecache.cpp include/synchronousimagecache.h
    imagecache/asynchronousimagecache.cpp include/asynchronousimagecache.h
    include/asynchronousimagecacheinterface.h
    imagecache/imagecachecollectorinterface.h
    imagecache/imagecachegenerator.cpp imagecache/imagecachegenerator.h
    imagecache/imagecachegeneratorinterface.h
    imagecache/imagecachestorage.h
    imagecache/imagecachestorageinterface.h
    imagecache/timestampproviderinterface.h

    include/qmldesignercorelib_global.h

  EXPLICIT_MOC
    "../mockup/qmldesigner/designercore/include/nodeinstanceview.h"
    "../mockup/qmldesigner/designercore/include/rewriterview.h"
    "${QmlDesignerDir}/designercore/include/model.h"
)

extend_qtc_test_with_target_sources(Utils DEFINES QTCREATOR_UTILS_STATIC_LIB "QTC_REL_TOOLS_PATH=\"${RELATIVE_LIBEXEC_PATH}\"")
extend_qtc_test(unittest
  CONDITION APPLE
  DEPENDS ${FWFoundation} ${FWAppKit}
)
extend_qtc_test_with_target_sources(CPlusPlus DEFINES CPLUSPLUS_BUILD_STATIC_LIB)
extend_qtc_test_with_target_sources(3rd_cplusplus DEFINES CPLUSPLUS_BUILD_LIB)
extend_qtc_test_with_target_sources(ClangSupport DEFINES CLANGSUPPORT_BUILD_LIB)

extend_qtc_test(unittest
  SOURCES_PREFIX_FROM_TARGET ClangCodeModel
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

extend_qtc_test(unittest
  SOURCES_PREFIX_FROM_TARGET CompilationDatabaseProjectManager
  SOURCES
    compilationdatabaseutils.cpp compilationdatabaseutils.h
)

extend_qtc_test(unittest
  CONDITION TARGET CompilationDatabaseProjectManager
  SOURCES
    compilationdatabaseutils-test.cpp
)

extend_qtc_test(unittest
  SOURCES_PREFIX_FROM_TARGET Core
  DEFINES CORE_STATIC_LIBRARY
  SOURCES
    coreicons.cpp coreicons.h
    find/ifindfilter.cpp find/ifindfilter.h
    locator/ilocatorfilter.cpp locator/ilocatorfilter.h
)

extend_qtc_test(unittest
  SOURCES_PREFIX_FROM_TARGET CppTools
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

extend_qtc_test(unittest
  SOURCES_PREFIX_FROM_TARGET ProjectExplorer
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

extend_qtc_test(unittest
  SOURCES_PREFIX_FROM_TARGET ClangFormat
  DEFINES CLANGPCHMANAGER_STATIC_LIB
  SOURCES
    clangformatconstants.h
    clangformatbaseindenter.cpp clangformatbaseindenter.h
)
