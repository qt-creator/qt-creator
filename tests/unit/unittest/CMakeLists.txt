find_package(Googletest MODULE)
find_package(GoogleBenchmark MODULE)
find_package(Qt6 COMPONENTS QmlDomPrivate QmlCompilerPrivate)

if (NOT Googletest_FOUND)
  message(STATUS "Googletest was not found. Please set GOOGLETEST_DIR (CMake or Environment) variable.")
  message(STATUS "Have a look at cmake/FindGoogletest.cmake file for more details.")
  message(STATUS "unittest module will be skipped.")
  return()
endif()

if (MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
elseif (MINGW)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
endif()

file(RELATIVE_PATH RELATIVE_TEST_PATH "${PROJECT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
file(RELATIVE_PATH TEST_RELATIVE_LIBEXEC_PATH "/${RELATIVE_TEST_PATH}" "/${IDE_LIBEXEC_PATH}")

add_qtc_test(unittest GTEST
  PROPERTIES COMPILE_WARNING_AS_ERROR OFF
  INCLUDES
    BEFORE "../mockup"
    BEFORE "../mockup/qmldesigner/designercore/include"
  DEPENDS
    Qt::Core Qt::Network Qt::Widgets
    Qt::Xml Qt::Concurrent Qt::Qml Qt::Gui
    Qt::Core5Compat QmlJS Sqlite SqliteC
    Googletest
  DEFINES
    QT_NO_CAST_TO_ASCII
    QT_RESTRICTED_CAST_FROM_ASCII
    UNIT_TESTS
    DONT_CHECK_MESSAGE_COUNTER
    QTC_RESOURCE_DIR="${CMAKE_CURRENT_LIST_DIR}/../../../share/qtcreator"
    TESTDATA_DIR="${CMAKE_CURRENT_BINARY_DIR}/data"
    TEST_RELATIVE_LIBEXEC_PATH="${TEST_RELATIVE_LIBEXEC_PATH}"
  SOURCES
    abstractviewmock.h
    conditionally-disabled-tests.h
    dynamicastmatcherdiagnosticcontainer-matcher.h
    eventspy.cpp eventspy.h
    fakeprocess.cpp fakeprocess.h
    googletest.h
    google-using-declarations.h
    gtest-creator-printing.cpp gtest-creator-printing.h
    gtest-llvm-printing.h
    gtest-qt-printing.cpp gtest-qt-printing.h
    gtest-std-printing.h
    lastchangedrowid-test.cpp
    matchingtext-test.cpp
    mockfutureinterface.h
    mockmutex.h
    mockqfilesystemwatcher.h
    mocksqlitestatement.h
    mocksqlitetransactionbackend.h
    mocksyntaxhighligher.h
    mocktimer.cpp mocktimer.h
    nodelistproperty-test.cpp
    processevents-utilities.cpp processevents-utilities.h
    sizedarray-test.cpp
    smallstring-test.cpp
    spydummy.cpp spydummy.h
    sqlitealgorithms-test.cpp
    sqliteindex-test.cpp
    sqliteteststatement.h
    sqlitetransaction-test.cpp
    unittests-main.cpp
    unittest-utility-functions.h
    sqlitecolumn-test.cpp
    sqlitedatabasebackend-test.cpp
    sqlitedatabase-test.cpp
    sqlitefunctionregistry-test.cpp
    sqlitesessions-test.cpp
    sqlitestatement-test.cpp
    sqlitetable-test.cpp
    sqlstatementbuilder-test.cpp
    createtablesqlstatementbuilder-test.cpp
    sqlitevalue-test.cpp
    asynchronousimagecache-test.cpp
    synchronousimagecache-test.cpp
    imagecachegenerator-test.cpp
    imagecachestorage-test.cpp
    imagecachedispatchcollector-test.cpp
    sqlitedatabasemock.h
    sqlitereadstatementmock.cpp sqlitereadstatementmock.h
    sqlitereadwritestatementmock.cpp
    sqlitestatementmock.h
    sqlitetransactionbackendmock.h
    sqlitewritestatementmock.cpp sqlitewritestatementmock.h
    notification.h
    mocktimestampprovider.h
    imagecachecollectormock.h
    mockimagecachegenerator.h
    mockimagecachestorage.h
    asynchronousexplicitimagecache-test.cpp
    asynchronousimagefactory-test.cpp
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

# Do not work on the source directory data
add_custom_command(TARGET unittest POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/data"
          "${CMAKE_CURRENT_BINARY_DIR}/data"
)

extend_qtc_test(unittest
  CONDITION TARGET GoogleBenchmark
  DEPENDS GoogleBenchmark
  SOURCES
    smallstring-benchmark.cpp
)

finalize_qtc_gtest(unittest
  EXCLUDE_SOURCES_REGEX ".c$"
  EXCLUDE_ALL_FROM_PRECHECK)

# Path needs to be before CppEditor
target_include_directories(unittest
  PRIVATE
    BEFORE ../../../src/plugins
)

# QmlDesigner tests

set(QmlDesignerDir ../../../src/plugins/qmldesigner)
extend_qtc_test(unittest
  INCLUDES
    "${QmlDesignerDir}"
    "${QmlDesignerDir}/designercore"
    "${QmlDesignerDir}/designercore/include"
    "${QmlDesignerDir}/designercore/imagecache"
    "${QmlDesignerDir}/../../../src/libs/qmlpuppetcommunication/interfaces"
    "${QmlDesignerDir}/../../../src/libs/qmlpuppetcommunication/types"
  DEFINES
    QMLDESIGNERCORE_STATIC_LIBRARY QMLDESIGNER_STATIC_LIBRARY
  SOURCES_PREFIX
    "${QmlDesignerDir}"
  SOURCES
    components/listmodeleditor/listmodeleditormodel.cpp components/listmodeleditor/listmodeleditormodel.h
)
extend_qtc_test(unittest
  SOURCES_PREFIX "${QmlDesignerDir}/designercore"
  SOURCES
    ../../../../src/libs/qmlpuppetcommunication/interfaces/commondefines.h
    ../components/listmodeleditor/listmodeleditormodel.cpp
    ../components/listmodeleditor/listmodeleditormodel.h
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
    imagecache/asynchronousexplicitimagecache.cpp
    imagecache/asynchronousimagecache.cpp
    imagecache/asynchronousimagefactory.cpp
    imagecache/asynchronousimagefactory.h
    imagecache/imagecachecollectorinterface.h
    imagecache/imagecachegenerator.cpp
    imagecache/imagecachegenerator.h
    imagecache/imagecachegeneratorinterface.h
    imagecache/imagecachestorage.h
    imagecache/imagecachedispatchcollector.h
    imagecache/imagecachestorageinterface.h
    imagecache/synchronousimagecache.cpp
    imagecache/timestampproviderinterface.h
    include/abstractproperty.h
    include/abstractview.h
    include/asynchronousexplicitimagecache.h
    include/asynchronousimagecache.h
    include/asynchronousimagecacheinterface.h
    include/bindingproperty.h
    include/imagecacheauxiliarydata.h
    include/import.h
    include/model.h
    include/modelnode.h
    include/nodeabstractproperty.h
    include/nodelistproperty.h
    include/nodeproperty.h
    include/projectstorageids.h
    include/qmldesignercorelib_global.h
    include/signalhandlerproperty.h
    include/synchronousimagecache.h
    include/variantproperty.h
    model/abstractproperty.cpp
    model/abstractview.cpp
    model/annotation.cpp
    model/bindingproperty.cpp
    model/import.cpp
    model/internalbindingproperty.cpp
    model/internalbindingproperty.h
    model/internalnode.cpp
    model/internalnode_p.h
    model/internalnodeabstractproperty.cpp
    model/internalnodeabstractproperty.h
    model/internalnodelistproperty.cpp
    model/internalnodelistproperty.h
    model/internalnodeproperty.cpp
    model/internalnodeproperty.h
    model/internalproperty.cpp
    model/internalproperty.h
    model/internalsignalhandlerproperty.cpp
    model/internalsignalhandlerproperty.h
    model/internalvariantproperty.cpp
    model/internalvariantproperty.h
    model/model.cpp
    model/model_p.h
    model/modelnode.cpp
    model/nodeabstractproperty.cpp
    model/nodelistproperty.cpp
    model/nodeproperty.cpp
    model/signalhandlerproperty.cpp
    model/variantproperty.cpp
    projectstorage/directorypathcompressor.h
    projectstorage/filesysteminterface.h
    projectstorage/filesystem.cpp projectstorage/filesystem.h
    projectstorage/filestatus.h
    projectstorage/filestatuscache.cpp projectstorage/filestatuscache.h
    projectstorage/nonlockingmutex.h
    projectstorage/projectstorageexceptions.cpp projectstorage/projectstorageexceptions.h
    projectstorage/projectstorageinterface.h
    projectstorage/projectstorage.cpp projectstorage/projectstorage.h
    projectstorage/projectstoragepathwatcher.h
    projectstorage/projectstoragepathwatcherinterface.h
    projectstorage/projectstoragepathwatchernotifierinterface.h
    projectstorage/projectstoragesqlitefunctionregistry.cpp
    projectstorage/projectstoragesqlitefunctionregistry.h
    projectstorage/projectstoragepathwatcher.h
    projectstorage/projectstoragepathwatchertypes.h
    projectstorage/projectstoragetypes.h
    projectstorage/projectstorageupdater.cpp projectstorage/projectstorageupdater.h
    projectstorage/sourcepath.h
    projectstorage/sourcepathcache.h
    projectstorage/sourcepathcache.h
    projectstorage/sourcepathcachetypes.h
    projectstorage/sourcepathview.h
    projectstorage/storagecache.h
    projectstorage/storagecacheentry.h
    projectstorage/storagecachefwd.h
    projectstorage/qmldocumentparserinterface.h
    projectstorage/qmltypesparserinterface.h
    rewritertransaction.cpp
    rewritertransaction.h
  EXPLICIT_MOC
    "../mockup/qmldesigner/designercore/include/nodeinstanceview.h"
    "../mockup/qmldesigner/designercore/include/rewriterview.h"
    "${QmlDesignerDir}/designercore/include/model.h"
)

extend_qtc_test(unittest
  SOURCES
    directorypathcompressor-test.cpp
    filesystemmock.h
    filestatuscache-test.cpp
    listmodeleditor-test.cpp
    projectstorage-test.cpp
    projectstorageupdater-test.cpp
    projectstoragesqlitefunctionregistry-test.cpp
    projectstoragepathwatchermock.h
    projectstoragepathwatchernotifiermock.h
    projectstoragepathwatcher-test.cpp
    sourcepath-test.cpp
    sourcepathcache-test.cpp
    sourcepathcachemock.h
    sourcepathview-test.cpp
    storagecache-test.cpp
    qmldocumentparsermock.h
    qmltypesparsermock.h
)

# QmlDesigner tests END

if (NOT TARGET Utils)
  add_subdirectory(../../../src/libs/utils ${CMAKE_CURRENT_BINARY_DIR}/utils)
endif()
if (NOT TARGET CPlusPlus)
  add_subdirectory(../../../src/libs/3rdparty/cplusplus ${CMAKE_CURRENT_BINARY_DIR}/3rd_cplusplus)
  add_subdirectory(../../../src/libs/cplusplus ${CMAKE_CURRENT_BINARY_DIR}/cplusplus)
endif()

extend_qtc_test(unittest DEPENDS Utils CPlusPlus)

extend_qtc_test(unittest
  CONDITION TARGET Qt6::QmlDomPrivate AND TARGET Qt6::QmlCompilerPrivate AND Qt6_VERSION VERSION_GREATER_EQUAL 6.5.0
  DEPENDS Qt6::QmlDomPrivate Qt6::QmlCompilerPrivate
  SOURCES
    qmldocumentparser-test.cpp
    qmltypesparser-test.cpp
)

extend_qtc_test(unittest
  CONDITION TARGET Qt6::QmlDomPrivate AND TARGET Qt6::QmlCompilerPrivate AND Qt6_VERSION VERSION_GREATER_EQUAL 6.5.0
  SOURCES_PREFIX "${QmlDesignerDir}/designercore"
  DEPENDS Qt6::QmlDomPrivate Qt6::QmlCompilerPrivate
  DEFINES QDS_HAS_QMLDOM
  SOURCES
    projectstorage/qmldocumentparser.cpp projectstorage/qmldocumentparser.h
    projectstorage/qmltypesparser.cpp projectstorage/qmltypesparser.h
)

file(GLOB PROJECTSTORAGE_EXCLUDED_SOURCES ${QmlDesignerDir}/designercore/projectstorage/*.cpp)
set_property(SOURCE ${PROJECTSTORAGE_EXCLUDED_SOURCES} PROPERTY SKIP_AUTOMOC ON)

file(GLOB UNITTEST_EXCLUDED_SOURCES *.cpp)
set_property(SOURCE ${UNITTEST_EXCLUDED_SOURCES} PROPERTY SKIP_AUTOMOC ON)
