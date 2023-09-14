cmake_minimum_required(VERSION 3.16)

## Add paths to check for cmake modules:
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Utils)

env_with_default("QTC_BUILD_WITH_PCH" ENV_QTC_BUILD_WITH_PCH ON)
env_with_default("QTC_WITH_TESTS" ENV_QTC_WITH_TESTS OFF)
env_with_default("QTC_WITH_QMLDESIGNER" ENV_QTC_WITH_QMLDESIGNER ON)

option(BUILD_WITH_PCH "Build with precompiled headers" "${ENV_QTC_BUILD_WITH_PCH}")

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

qtc_handle_compiler_cache_support()

option(BUILD_LINK_WITH_QT "Link with Qt from the parent Qt Creator" OFF)
qtc_link_with_qt()

option(WITH_TESTS "Build Tests" ${ENV_QTC_WITH_TESTS})
add_feature_info("Build tests" ${WITH_TESTS} "")
option(WITH_DEBUG_CMAKE "Enabled CMake project debugging functionality" OFF)
option(SHOW_BUILD_DATE "Show build date in about dialog" OFF)
option(WITH_SANITIZE "Build with sanitizer enabled" OFF)
set(SANITIZE_FLAGS "" CACHE STRING "Sets flags for sanitizer compilation flags used in Debug builds")
add_feature_info("Build with sanitize" ${WITH_SANITIZE} "SANITIZE_FLAGS='${SANITIZE_FLAGS}'")


# merge binary directories of sub projects into top level
set(QTC_MERGE_BINARY_DIR ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Set up Qt stuff:
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if (WITH_TESTS)
  set(QT_TEST_COMPONENT Test)
  set(IMPLICIT_DEPENDS Qt::Test)
endif()

# suppress bogus warning
# warning C4573 requires the compiler to capture 'this' but the current default capture mode does not allow it
# when using QObject::connect in lambda without capturing 'this' - which would lead to warnings
# with other compilers
if(MSVC)
  add_compile_options(/wd4573)
endif()

find_package(Qt6
  ${IDE_QT_VERSION_MIN}
  COMPONENTS Concurrent Core Gui Network PrintSupport Qml Sql Widgets Xml Core5Compat ${QT_TEST_COMPONENT}
  REQUIRED
)

if (Qt6_VERSION VERSION_GREATER_EQUAL 6.4.3)
    option(WITH_QMLDESIGNER "Build QmlDesigner" ${ENV_QTC_WITH_QMLDESIGNER})
else()
    option(WITH_QMLDESIGNER "Build QmlDesigner" OFF)
endif()
add_feature_info("Build QmlDesigner and related code (only if Qt is 6.4.3 or newer)" ${WITH_QMLDESIGNER} "")

# hack for Qbs which still supports Qt5 and Qt6
if (TARGET Qt6::Core5CompatPrivate)
    if (CMAKE_VERSION VERSION_LESS 3.18)
        set_property(TARGET Qt6::Core5CompatPrivate PROPERTY IMPORTED_GLOBAL TRUE)
    endif()
    add_library(Qt6Core5CompatPrivate ALIAS Qt6::Core5CompatPrivate)
endif()
if (TARGET Qt6::Core5Compat)
    if (CMAKE_VERSION VERSION_LESS 3.18)
        set_property(TARGET Qt6::Core5Compat PROPERTY IMPORTED_GLOBAL TRUE)
    endif()
    add_library(Qt6Core5Compat ALIAS Qt6::Core5Compat)
endif()

# Common intermediate directory for QML modules which are defined via qt_add_qml_module()
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml_modules")

# This includes the code that will enable higher compiler warnings level (/W3 for MSVC, -Wall -Wextra for GCC)
# This is controlled by QT_COMPILE_OPTIONS_DISABLE_WARNINGS target property.
include(QtCompilerFlags)

if (MSVC AND QT_FEATURE_static_runtime)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

find_package(Qt6 OPTIONAL_COMPONENTS Quick QuickWidgets Designer DesignerComponentsPrivate
                                     Help SerialPort Svg Tools LinguistTools QUIET)

find_package(Threads)
find_package(Clang QUIET)

# Crashpad
set(CRASHPAD_BACKEND_URL "" CACHE STRING "Crashpad backend URL")
set(BUILD_WITH_CRASHPAD OFF)
# Linux is not supported for now
# x86_64;arm64 is not supported for now
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

set_if_target(_has_svg_target Qt::Svg)
option(ENABLE_SVG_SUPPORT "Enable SVG support" "${_has_svg_target}")

add_library(OptionalSvg INTERFACE)
if (TARGET Qt::Svg AND ENABLE_SVG_SUPPORT)
  target_link_libraries(OptionalSvg INTERFACE Qt::Svg)
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
add_subdirectory(dist)

if (WITH_TESTS)
  add_subdirectory(tests)
endif()

add_subdirectory(doc)

setup_dependencies_component()

feature_summary(INCLUDE_QUIET_PACKAGES WHAT
  PACKAGES_FOUND PACKAGES_NOT_FOUND
  ENABLED_FEATURES DISABLED_FEATURES
)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR
    AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.GPL3-EXCEPT")
  # Only for opensource, non-super-repo builds
  add_subdirectory(packaging)
endif()
