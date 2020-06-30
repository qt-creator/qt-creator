cmake_minimum_required(VERSION 3.10)

## Add paths to check for cmake modules:
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(FeatureSummary)
include(QtCreatorIDEBranding)
include(QtCreatorTranslations)
include(QtCreatorDocumentation)

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
  COMPONENTS Concurrent Core Gui Network PrintSupport Qml Quick
    QuickWidgets Sql Widgets Xml ${QT_TEST_COMPONENT}
  REQUIRED
)

find_package(Qt5 COMPONENTS LinguistTools)

find_package(Threads)

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

# CMake will include in a cmake_install.cmake at the end the subdirectories
# At this point all the previous install scripts have been included
# Deployment is being done in cmake/CMakeLists.txt
add_subdirectory(cmake)

feature_summary(INCLUDE_QUIET_PACKAGES WHAT
  PACKAGES_FOUND PACKAGES_NOT_FOUND
  ENABLED_FEATURES DISABLED_FEATURES
)
