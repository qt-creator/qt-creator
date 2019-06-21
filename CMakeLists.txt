cmake_minimum_required(VERSION 3.9)

include(FeatureSummary)

#BINARY_ARTIFACTS_BRANCH = master
#PROJECT_USER_FILE_EXTENSION = .user

set(IDE_VERSION "4.10.82" CACHE STRING "The IDE version.")
set(IDE_VERSION_COMPAT "4.10.82" CACHE STRING "The IDE Compatibility version.")
set(IDE_VERSION_DISPLAY "4.11.0-beta1" CACHE STRING "The IDE display version.")
set(IDE_COPYRIGHT_YEAR "2019" CACHE STRING "The IDE copyright year.")

set(IDE_REVISION FALSE CACHE BOOL "Marks the presence of IDE revision string.")
set(IDE_REVISION_STR "" CACHE STRING "The IDE revision string.")
set(IDE_SETTINGSVARIANT "QtProject" CACHE STRING "The IDE settings variation.")
set(IDE_COPY_SETTINGSVARIANT "Nokia" CACHE STRING "The IDE settings to initially import.")
set(IDE_DISPLAY_NAME "Qt Creator" CACHE STRING "The IDE display name.")
set(IDE_ID "qtcreator" CACHE STRING "The IDE id (no spaces, lowercase!)")
set(IDE_CASED_ID "QtCreator" CACHE STRING "The cased IDE id (no spaces!)")
set(IDE_BUNDLE_IDENTIFIER "org.qt-project.${IDE_ID}" CACHE STRING "The macOS application bundle identifier.")

mark_as_advanced(IDE_VERSION_COMPAT IDE_VERSION_DISPLAY IDE_COPYRIGHT_YEAR
  IDE_REVISION IDE_REVISION_STR IDE_SETTINGSVARIANT IDE_COPY_SETTINGSVARIANT
  IDE_DISPLAY_NAME IDE_ID IDE_CASED_ID IDE_BUNDLE_IDENTIFIER)

project(QtCreator VERSION ${IDE_VERSION})

## Add paths to check for cmake modules:
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Force C++ standard, do not fall back, do not use compiler extensions
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(WITH_TESTS "Build Tests" OFF)
option(WITH_DEBUG_CMAKE "Enabled CMake project debugging functionality (e.g. source file disk checking)" OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Set up Qt stuff:
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if (WITH_TESTS)
  set(_TEST_QT_COMPONENT Test)
  set(_TEST_DEPENDS Qt5::Test)
endif()

find_package(Qt5
  COMPONENTS Concurrent Core Network PrintSupport Qml Quick QuickWidgets
    Sql ${_TEST_QT_COMPONENT}
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

find_package(LLVM QUIET)
find_package(Clang COMPONENTS libclang QUIET)

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

set(_IDE_APP_PATH "bin")

if (APPLE)
  set(_IDE_APP_TARGET "Qt Creator")

  set(_IDE_OUTPUT_PATH "${_IDE_APP_PATH}/${_IDE_APP_TARGET}.app/Contents")

  set(_IDE_PLUGIN_PATH "${_IDE_OUTPUT_PATH}/PlugIns")
  set(_IDE_LIBRARY_BASE_PATH "Frameworks")
  set(_IDE_LIBRARY_PATH "${_IDE_OUTPUT_PATH}/Frameworks")
  set(_IDE_LIBEXEC_PATH "${_IDE_OUTPUT_PATH}/Resources")
  set(_IDE_DATA_PATH "${_IDE_OUTPUT_PATH}/Resources")
  set(_IDE_DOC_PATH "${_IDE_OUTPUT_PATH}/Resources/doc")
  set(_IDE_BIN_PATH "${_IDE_OUTPUT_PATH}/MacOS")
else ()
  set(_IDE_APP_TARGET "qtcreator")

  set(_IDE_LIBRARY_BASE_PATH "lib")
  set(_IDE_LIBRARY_PATH "lib/qtcreator")
  set(_IDE_PLUGIN_PATH "lib/qtcreator/plugins")
  if (WIN32)
    set(_IDE_LIBEXEC_PATH "bin")
  else ()
    set(_IDE_LIBEXEC_PATH "libexec/qtcreator/bin")
  endif ()
  set(_IDE_DATA_PATH "share/qtcreator")
  set(_IDE_DOC_PATH "share/doc/qtcreator")
  set(_IDE_BIN_PATH "bin")
endif ()

set(IDE_APP_PATH "${_IDE_APP_PATH}" CACHE PATH "The target path of the IDE application (relative to CMAKE_INSTALL_PREFIX).")
set(IDE_APP_TARGET "${_IDE_APP_TARGET}" CACHE PATH "The IDE application name.")
set(IDE_PLUGIN_PATH "${_IDE_PLUGIN_PATH}" CACHE PATH "The IDE plugin path (relative to CMAKE_INSTALL_PREFIX).")
set(IDE_LIBRARY_BASE_PATH "${_IDE_LIBRARY_BASE_PATH}" CACHE PATH "The IDE library base path (relative to CMAKE_INSTALL_PREFIX).")
set(IDE_LIBRARY_PATH "${_IDE_LIBRARY_PATH}" CACHE PATH "The IDE library path (relative to CMAKE_INSTALL_PREFIX).")
set(IDE_LIBEXEC_PATH "${_IDE_LIBEXEC_PATH}" CACHE PATH "The IDE libexec path (relative to CMAKE_INSTALL_PREFIX).")
set(IDE_DATA_PATH "${_IDE_DATA_PATH}" CACHE PATH "The IDE data path (relative to CMAKE_INSTALL_PREFIX).")
set(IDE_DOC_PATH "${_IDE_DOC_PATH}" CACHE PATH "The IDE documentation path (relative to CMAKE_INSTALL_PREFIX).")
set(IDE_BIN_PATH "${_IDE_BIN_PATH}" CACHE PATH "The IDE bin path (relative to CMAKE_INSTALL_PREFIX).")

file(RELATIVE_PATH RELATIVE_PLUGIN_PATH "/${IDE_BIN_PATH}" "/${IDE_PLUGIN_PATH}")
file(RELATIVE_PATH RELATIVE_LIBEXEC_PATH "/${IDE_BIN_PATH}" "/${IDE_LIBEXEC_PATH}")
file(RELATIVE_PATH RELATIVE_DATA_PATH "/${IDE_BIN_PATH}" "/${IDE_DATA_PATH}")
file(RELATIVE_PATH RELATIVE_DOC_PATH "/${IDE_BIN_PATH}" "/${IDE_DOC_PATH}")

list(APPEND DEFAULT_DEFINES
  RELATIVE_PLUGIN_PATH="${RELATIVE_PLUGIN_PATH}"
  RELATIVE_LIBEXEC_PATH="${RELATIVE_LIBEXEC_PATH}"
  RELATIVE_DATA_PATH="${RELATIVE_DATA_PATH}"
  RELATIVE_DOC_PATH="${RELATIVE_DOC_PATH}"
)

file(RELATIVE_PATH _PLUGIN_TO_LIB "/${IDE_PLUGIN_PATH}" "/${IDE_LIBRARY_PATH}")

if (APPLE)
  set(_RPATH_BASE "@executable_path")
  set(_LIB_RPATH "@loader_path")
  set(_PLUGIN_RPATH "@loader_path;@loader_path/${_PLUGIN_TO_LIB}")
elseif (WIN32)
  set(_RPATH_BASE "")
  set(_LIB_RPATH "")
  set(_PLUGIN_RPATH "")
else()
  set(_RPATH_BASE "\$ORIGIN")
  set(_LIB_RPATH "\$ORIGIN")
  set(_PLUGIN_RPATH "\$ORIGIN;\$ORIGIN/${_PLUGIN_TO_LIB}")
endif ()

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
