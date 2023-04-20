cmake_minimum_required(VERSION 3.10)

#! [1]
# Remove when sharing with others.
list(APPEND CMAKE_PREFIX_PATH "/Users/example/qt-creator/build")
#! [1]

#! [2]
project(Example)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)
#! [2]

#! [3]
find_package(QtCreator COMPONENTS Core REQUIRED)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
set(QtX Qt${QT_VERSION_MAJOR})
#! [3]

#! [5]
# Add a CMake option that enables building your plugin with tests.
# You don't want your released plugin binaries to contain tests,
# so make that default to 'NO'.
# Enable tests by passing -DWITH_TESTS=ON to CMake.
option(WITH_TESTS "Builds with tests" NO)

if(WITH_TESTS)
  # Look for QtTest
  find_package(${QtX} REQUIRED COMPONENTS Test)
  # Tell CMake functions like add_qtc_plugin about the QtTest component.
  set(IMPLICIT_DEPENDS Qt::Test)

  # Enable ctest for auto tests.
  enable_testing()
endif()
#! [5]

#! [4]
add_qtc_plugin(Example
  PLUGIN_DEPENDS
    QtCreator::Core
  DEPENDS
    ${QtX}::Widgets
    QtCreator::ExtensionSystem
    QtCreator::Utils
  SOURCES
    .github/workflows/build_cmake.yml
    .github/workflows/README.md
    README.md
    example.cpp
    example.h
    example_global.h
    exampleconstants.h
    examplefunctions.h
)
#! [4]

#! [6]
# conditionally add auto tests
if(WITH_TESTS)
  add_qtc_test(tst_mytest
    SOURCES tst_mytest.cpp
    DEPENDS Example
  )
endif()
#! [6]
