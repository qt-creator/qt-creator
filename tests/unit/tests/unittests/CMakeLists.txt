file(RELATIVE_PATH RELATIVE_TEST_PATH "${PROJECT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
file(RELATIVE_PATH TEST_RELATIVE_LIBEXEC_PATH "/${RELATIVE_TEST_PATH}" "/${IDE_LIBEXEC_PATH}")

add_qtc_test(unittest GTEST
  PROPERTIES COMPILE_WARNING_AS_ERROR OFF
  SKIP_AUTOGEN ON
  DEPENDS
    Qt::Core Qt::Network Qt::Widgets
    Qt::Xml Qt::Concurrent Qt::QmlPrivate Qt::Gui
    Qt::Core5Compat Utils QmlJS Sqlite
    Googletest TestDesignerCore TestUtils TestMatchers TestPrinters TestMocks
  DEFINES
    GTEST_INTERNAL_HAS_STRING_VIEW
    QT_NO_CAST_TO_ASCII
    QT_RESTRICTED_CAST_FROM_ASCII
    UNIT_TESTS
    DONT_CHECK_MESSAGE_COUNTER
    QTC_RESOURCE_DIR="${QtCreatorResourcesDir}"
    UNITTEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    TEST_RELATIVE_LIBEXEC_PATH="${TEST_RELATIVE_LIBEXEC_PATH}"
    QT6_INSTALL_PREFIX="${QT6_INSTALL_PREFIX}"
    QDS_MODEL_USE_PROJECTSTORAGEINTERFACE
    QDS_USE_PROJECTSTORAGE
  SOURCES
    unittests-main.cpp
)

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

finalize_qtc_gtest(unittest
  EXCLUDE_SOURCES_REGEX ".c$"
  EXCLUDE_ALL_FROM_PRECHECK
)

function(unittest_copy_data_folder)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/data"
    "${CMAKE_CURRENT_BINARY_DIR}/data"
  )
endfunction(unittest_copy_data_folder)

add_subdirectory(listmodeleditor)
add_subdirectory(imagecache)
add_subdirectory(metainfo)
add_subdirectory(model)
add_subdirectory(sqlite)
add_subdirectory(projectstorage)
add_subdirectory(qmlprojectmanager)
add_subdirectory(utils)
