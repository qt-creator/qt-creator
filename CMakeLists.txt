cmake_minimum_required(VERSION 3.10)

## Add paths to check for cmake modules:
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(FeatureSummary)
include(QtCreatorIDEBranding RESULT_VARIABLE IDE_BRANDING_FILE)
include(QtCreatorTranslations)
include(QtCreatorDocumentation)
include(QtCreatorAPI)

set(IDE_REVISION FALSE CACHE BOOL "Marks the presence of IDE revision string.")
set(IDE_REVISION_STR "" CACHE STRING "The IDE revision string.")
set(IDE_REVISION_URL "" CACHE STRING "The IDE revision Url string.")

mark_as_advanced(IDE_REVISION IDE_REVISION_STR IDE_REVISION_URL)

project(QtCreator VERSION ${IDE_VERSION})

# Force C++ standard, do not fall back, do not use compiler extensions
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(WITH_TESTS "Build Tests" OFF)
add_feature_info("Build tests" ${WITH_TESTS} "")
option(WITH_DEBUG_CMAKE "Enabled CMake project debugging functionality (e.g. source file disk checking)" OFF)

# merge binary directories of sub projects into top level
set(QTC_MERGE_BINARY_DIR ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Set up Qt stuff:
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if (WITH_TESTS)
  set(QT_TEST_COMPONENT Test)
  set(IMPLICIT_DEPENDS Qt5::Test)
endif()

find_package(Qt5
  ${IDE_QT_VERSION_MIN}
  COMPONENTS Concurrent Core Gui Network PrintSupport Qml Quick
    QuickWidgets Sql Widgets Xml Core5Compat ${QT_TEST_COMPONENT}
  REQUIRED
)
if (Qt5_VERSION VERSION_LESS 6.0.0)
  install(TARGETS Qt6Core5Compat EXPORT QtCreator)
  set(BUILD_WITH_PCH_DEFAULT ON)
else()
  set(BUILD_WITH_PCH_DEFAULT OFF)
endif()
find_package(Qt5 COMPONENTS LinguistTools QUIET)
find_package(Qt5 COMPONENTS Designer DesignerComponents Help SerialPort Svg Tools QUIET)

option(BUILD_WITH_PCH "Build with precompiled headers" ${BUILD_WITH_PCH_DEFAULT})

find_package(Threads)
find_package(Clang QUIET)
find_package(KF5SyntaxHighlighting QUIET)

# Crashpad
set(CRASHPAD_BACKEND_URL "" CACHE STRING "Crashpad backend URL")
set(BUILD_WITH_CRASHPAD OFF)
if(CRASHPAD_BACKEND_URL AND (WIN32 OR APPLE)) # Linux is not supported for now
  find_package(Crashpad QUIET)
  if(TARGET Crashpad::Crashpad)
    set(BUILD_WITH_CRASHPAD ON)
  endif()
endif()
add_feature_info("Build with Crashpad" ${BUILD_WITH_CRASHPAD} "")

function (set_if_target var target)
   if (TARGET "${target}")
     set(_result ON)
   else()
     set(_result OFF)
   endif()
   set(${var} "${_result}" PARENT_SCOPE)
endfunction()

set_if_target(_has_svg_target Qt5::Svg)
option(ENABLE_SVG_SUPPORT "Enable SVG support" "${_has_svg_target}")

# specify standards conformance mode to MSVC 2017 and later
if (MSVC AND MSVC_VERSION GREATER_EQUAL 1910)
    add_compile_options(/permissive-)
endif()

add_library(OptionalSvg INTERFACE)
if (TARGET Qt5::Svg AND ENABLE_SVG_SUPPORT)
  target_link_libraries(OptionalSvg INTERFACE Qt5::Svg)
else()
  target_compile_definitions(OptionalSvg INTERFACE QT_NO_SVG)
endif()
install(TARGETS OptionalSvg EXPORT QtCreator)

if (APPLE)
  find_library(FWCoreFoundation CoreFoundation)
  find_library(FWCoreServices CoreServices)
  find_library(FWFoundation Foundation)
  find_library(FWAppKit AppKit)
  find_library(FWIOKit IOKit)
  find_library(FWSecurity Security)
  find_library(FWSystemConfiguration SystemConfiguration)
  find_library(FWWebKit WebKit)
endif()

if (WITH_TESTS)
  enable_testing()
endif()

if (UNIX)
  add_subdirectory(bin)
endif()

add_subdirectory(src)
add_subdirectory(share)

if (WITH_TESTS)
  add_subdirectory(tests)
endif()

add_subdirectory(doc)

find_package(PythonInterp)
if (NOT PYTHONINTERP_FOUND)
  message("No python interpreter found, skipping \"Dependencies\" install component.")
else()
  get_target_property(_qmake_binary Qt5::qmake IMPORTED_LOCATION)
  set(_llvm_arg)
  if (LLVM_INSTALL_PREFIX)
    set(_llvm_arg "--llvm \"${LLVM_INSTALL_PREFIX}\"")
  endif()
  set(_elfutils_arg)
  if (ELFUTILS_INCLUDE_DIR)
    get_filename_component(_elfutils_path ${ELFUTILS_INCLUDE_DIR} DIRECTORY)
    set(_elfutils_arg "--elfutils \"${_elfutils_path}\"")
  endif()
  install(CODE "
      execute_process(COMMAND
        \"${PYTHON_EXECUTABLE}\"
        \"${CMAKE_CURRENT_LIST_DIR}/scripts/deployqt.py\"
        ${_llvm_arg}
        ${_elfutils_arg}
        \"\${CMAKE_INSTALL_PREFIX}/${IDE_APP_PATH}/${IDE_APP_TARGET}\"
        \"${_qmake_binary}\"
        COMMAND_ECHO STDOUT
        )
    "
    COMPONENT Dependencies
    EXCLUDE_FROM_ALL
  )
endif()

# install qmake support files
install(
  FILES
    qtcreator.pri
    qtcreatordata.pri
    qtcreator_ide_branding.pri
  DESTINATION include
  COMPONENT Devel EXCLUDE_FROM_ALL
)
install(
  FILES
    src/qtcreatorlibrary.pri
    src/qtcreatorplugin.pri
    src/qtcreatortool.pri
    src/rpath.pri
  DESTINATION include/src
  COMPONENT Devel EXCLUDE_FROM_ALL
)

feature_summary(INCLUDE_QUIET_PACKAGES WHAT
  PACKAGES_FOUND PACKAGES_NOT_FOUND
  ENABLED_FEATURES DISABLED_FEATURES
)
