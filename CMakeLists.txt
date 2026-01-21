cmake_minimum_required(VERSION 3.20)

option(BUILD_DESIGNSTUDIO "Build and install design studio plugins, examples and settings." OFF)
if (BUILD_DESIGNSTUDIO)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/dist/branding/qtdesignstudio")
endif()

## Add paths to check for cmake modules:
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Utils)

env_with_default("QTC_BUILD_WITH_PCH" ENV_QTC_BUILD_WITH_PCH ON)
env_with_default("QTC_WITH_TESTS" ENV_QTC_WITH_TESTS OFF)

option(BUILD_WITH_PCH "Build with precompiled headers" "${ENV_QTC_BUILD_WITH_PCH}")

include(FeatureSummary)
include(QtCreatorIDEBranding RESULT_VARIABLE IDE_BRANDING_FILE)
include(QtCreatorTranslations)
include(QtCreatorDocumentation)
include(QtCreatorAPI)
include(QtCreatorSbom)

set(IDE_REVISION FALSE CACHE BOOL "Marks the presence of IDE revision string.")
set(IDE_REVISION_STR "" CACHE STRING "The IDE revision string.")
set(IDE_REVISION_URL "" CACHE STRING "The IDE revision Url string.")

mark_as_advanced(IDE_REVISION IDE_REVISION_STR IDE_REVISION_URL)

project(QtCreator VERSION ${IDE_VERSION})

# Force C++ standard, do not fall back, do not use compiler extensions
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

qtc_handle_compiler_cache_support()
qtc_handle_llvm_linker()

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

if (MSVC)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${IDE_MSVC_VERSION_MIN})
    message(FATAL_ERROR "You need at least MSVC2022 to build Qt Creator.")
  endif()
endif()

find_package(Qt6
  ${IDE_QT_VERSION_MIN}
  COMPONENTS Concurrent Core Gui Network PrintSupport Qml Sql Widgets Xml ${QT_TEST_COMPONENT}
  REQUIRED
)

find_package(Qt6 OPTIONAL_COMPONENTS Quick QuickWidgets Designer DesignerComponentsPrivate
                                     Help SerialPort Svg Tools LinguistTools QUIET)

if (Qt6_VERSION VERSION_GREATER_EQUAL 6.9.0)
  find_package(Qt6 OPTIONAL_COMPONENTS
    CorePrivate
    GuiPrivate
    DesignerComponentsPrivate
    QmlPrivate
    QuickPrivate
    QUIET
  )
endif()

#if (Qt6_VERSION VERSION_GREATER_EQUAL 6.11.0)
#  find_package(Qt6
#    COMPONENTS
#    TaskTree
#    REQUIRED
#  )
#endif()

qtc_setup_sbom()
qtc_sbom_begin_project()

# depending on Qt version and compiler version enable or disable Qml Designer
# can be overwritten by variable WITH_QMLDESIGNER / QTC_WITH_QMLDESIGNER (env)
configure_qml_designer(${Qt6_VERSION})

# Common intermediate directory for QML modules which are defined via qt_add_qml_module()
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml_modules")

# This includes the code that will enable higher compiler warnings level (/W3 for MSVC, -Wall -Wextra for GCC)
# This is controlled by QT_COMPILE_OPTIONS_DISABLE_WARNINGS target property.
include(QtCompilerFlags)

if (MSVC AND QT_FEATURE_static_runtime)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

find_package(Threads)
find_package(Clang QUIET)

set(SENTRY_DSN "$ENV{QTC_SENTRY_DSN}" CACHE STRING "Data Source Name to use for sending Sentry events.")
set(SENTRY_PROJECT "" CACHE STRING "Project ID to use for sending Sentry events.")
set(BUILD_WITH_SENTRY OFF)
if(SENTRY_DSN AND SENTRY_PROJECT)
  find_package(sentry QUIET)
  if(TARGET sentry::sentry)
    set(BUILD_WITH_SENTRY ON)
    get_target_property(__sentry_configs sentry::sentry IMPORTED_CONFIGURATIONS)
    get_target_property(__sentry_lib sentry::sentry IMPORTED_LOCATION_${__sentry_configs})
    install(FILES ${__sentry_lib}
      DESTINATION "${IDE_LIBRARY_ARCHIVE_PATH}"
      COMPONENT Dependencies
      EXCLUDE_FROM_ALL
    )
    if (TARGET sentry_crashpad::crashpad_handler)
      get_target_property(SENTRY_CRASHPAD_PATH sentry_crashpad::crashpad_handler IMPORTED_LOCATION_${__sentry_configs})
      install(PROGRAMS ${SENTRY_CRASHPAD_PATH}
        DESTINATION "${IDE_LIBEXEC_PATH}"
        COMPONENT Dependencies
        EXCLUDE_FROM_ALL
      )
    endif()
  endif()
endif()
add_feature_info("Build with Sentry" ${BUILD_WITH_SENTRY} "")

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
if (NOT BUILD_DESIGNSTUDIO)
    add_subdirectory(dist)
endif()

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
    AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSES/LICENSE.GPL3-EXCEPT")
  # Only for opensource, non-super-repo builds
  add_subdirectory(packaging)
endif()

qtc_sbom_end_project()
