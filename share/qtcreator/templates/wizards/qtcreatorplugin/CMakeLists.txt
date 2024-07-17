cmake_minimum_required(VERSION 3.16)

# Remove when sharing with others.
@if %{JS: Util.isDirectory('%{QtCreatorBuild}/Qt Creator.app/Contents/Resources/lib/cmake/QtCreator')}
list(APPEND CMAKE_PREFIX_PATH "%{QtCreatorBuild}/Qt Creator.app/Contents/Resources")
@else
  @if %{JS: Util.isDirectory('%{QtCreatorBuild}/Contents/Resources/lib/cmake/QtCreator')}
list(APPEND CMAKE_PREFIX_PATH "%{QtCreatorBuild}/Contents/Resources")
  @else
list(APPEND CMAKE_PREFIX_PATH "%{QtCreatorBuild}")
  @endif
@endif

project(%{PluginName})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(QtCreator REQUIRED COMPONENTS Core)
find_package(Qt6 COMPONENTS Widgets REQUIRED)

# Add a CMake option that enables building your plugin with tests.
# You don't want your released plugin binaries to contain tests,
# so make that default to 'NO'.
# Enable tests by passing -DWITH_TESTS=ON to CMake.
option(WITH_TESTS "Builds with tests" NO)

if(WITH_TESTS)
  # Look for QtTest
  find_package(Qt6 REQUIRED COMPONENTS Test)

  # Tell CMake functions like add_qtc_plugin about the QtTest component.
  set(IMPLICIT_DEPENDS Qt::Test)

  # Enable ctest for auto tests.
  enable_testing()
endif()

add_qtc_plugin(%{PluginName}
  PLUGIN_DEPENDS
    QtCreator::Core
  DEPENDS
    Qt::Widgets
    QtCreator::ExtensionSystem
    QtCreator::Utils
  SOURCES
    .github/workflows/build_cmake.yml
    .github/workflows/README.md
    README.md
    %{SrcFileName}
    %{ConstantsHdrFileName}
    %{TrHdrFileName}
)
