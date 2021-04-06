cmake_minimum_required(VERSION 3.10)

# Remove when sharing with others.
@if %{JS: Util.isDirectory('%{QtCreatorBuild}/Qt Creator.app/Contents/Resources')}
list(APPEND CMAKE_PREFIX_PATH "%{QtCreatorBuild}/Qt Creator.app/Contents/Resources")
@else
  @if %{JS: Util.isDirectory('%{QtCreatorBuild}/Contents/Resources')}
list(APPEND CMAKE_PREFIX_PATH "%{QtCreatorBuild}/Contents/Resources")
  @else
list(APPEND CMAKE_PREFIX_PATH "%{QtCreatorBuild}")
  @endif
@endif

project(%{PluginName})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)

find_package(QtCreator COMPONENTS Core REQUIRED)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
set(QtX Qt${QT_VERSION_MAJOR})

add_qtc_plugin(%{PluginName}
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
    %{SrcFileName}
    %{HdrFileName}
    %{GlobalHdrFileName}
    %{ConstantsHdrFileName}
)
