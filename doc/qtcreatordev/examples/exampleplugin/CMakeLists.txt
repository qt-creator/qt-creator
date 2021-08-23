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
)
#! [4]
