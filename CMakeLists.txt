cmake_minimum_required(VERSION 3.9)

## Add paths to check for cmake modules:
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(FeatureSummary)
include(QtCreatorIDEBranding)

set(IDE_REVISION FALSE CACHE BOOL "Marks the presence of IDE revision string.")
set(IDE_REVISION_STR "" CACHE STRING "The IDE revision string.")

mark_as_advanced(IDE_REVISION IDE_REVISION_STR)

project(QtCreator VERSION ${IDE_VERSION})

# Force C++ standard, do not fall back, do not use compiler extensions
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(WITH_TESTS "Build Tests" OFF)
option(WITH_DEBUG_CMAKE "Enabled CMake project debugging functionality (e.g. source file disk checking)" OFF)
option(BUILD_WITH_PCH "Build with precompiled headers" ON)

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
  COMPONENTS Concurrent Core Gui LinguistTools Network PrintSupport Qml Quick
    QuickWidgets Sql Widgets Xml ${QT_TEST_COMPONENT}
  REQUIRED
)

find_package(Threads)

# Get information on directories from qmake
# as this is not yet exported by cmake.
function(qt5_query_qmake)
  if (NOT TARGET Qt5::qmake)
    message(FATAL_ERROR "Qmake was not found.")
  endif()

  get_target_property(_qmake_binary Qt5::qmake IMPORTED_LOCATION)
  execute_process(COMMAND "${_qmake_binary}" "-query"
                  TIMEOUT 10
                  RESULT_VARIABLE _qmake_result
                  OUTPUT_VARIABLE _qmake_stdout
                  OUTPUT_STRIP_TRAILING_WHITESPACE)

  if (NOT "${_qmake_result}" STREQUAL "0")
    message(FATAL_ERROR "Qmake did not execute successfully: ${_qmake_result}.")
  endif()

  # split into lines:
  string(REPLACE "\n" ";" _lines "${_qmake_stdout}")

  foreach(_line ${_lines})
    # split line into key/value pairs
    string(REPLACE ":" ";" _parts "${_line}")
    list(GET _parts 0 _key)
    list(REMOVE_AT _parts 0)
    string(REPLACE ";" ":" _value "${_parts}")

    set("${_key}" "${_value}" CACHE PATH "qmake import of ${_key}" FORCE)
  endforeach()
endfunction()

qt5_query_qmake()

find_package(Qt5 COMPONENTS Designer Help Script SerialPort Svg QUIET)
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

add_library(OptionalSvg INTERFACE)
if (TARGET Qt5::Svg AND ENABLE_SVG_SUPPORT)
  target_link_libraries(OptionalSvg INTERFACE Qt5::Svg)
else()
  target_compile_definitions(OptionalSvg INTERFACE QT_NO_SVG)
endif()
install(TARGETS OptionalSvg EXPORT QtCreator)

find_package(Clang COMPONENTS libclang QUIET)
# silence a lot of warnings from building against llvm
# this would better fit inside a central libclang detection/include cmake file, but since we do not
# have one put it temporary here
if(MSVC AND TARGET libclang)
    target_compile_options(libclang INTERFACE /wd4100 /wd4141 /wd4146 /wd4244 /wd4267 /wd4291)
endif()
find_package(LLVM QUIET)

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

include(QtCreatorAPI)

if (UNIX)
  add_subdirectory(bin)
endif()

add_subdirectory(src)
add_subdirectory(share)

if (WITH_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

add_subdirectory(doc)

feature_summary(INCLUDE_QUIET_PACKAGES WHAT
  PACKAGES_FOUND PACKAGES_NOT_FOUND
  ENABLED_FEATURES DISABLED_FEATURES
)
