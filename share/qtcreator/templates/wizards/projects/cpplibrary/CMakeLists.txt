cmake_minimum_required(VERSION 3.14)

project(%{ProjectName} LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
@if '%{QtModule}' != 'none'
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
@endif
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
@if '%{QtModule}' != 'none'

@if %{HasTranslation}
find_package(QT NAMES Qt6 Qt5 COMPONENTS %{QtModuleUpperCase} LinguistTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS %{QtModuleUpperCase} LinguistTools REQUIRED)

set(TS_FILES %{TsFileName})
@else
find_package(QT NAMES Qt6 Qt5 COMPONENTS %{QtModuleUpperCase} REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS %{QtModuleUpperCase} REQUIRED)
@endif
@endif

add_library(%{ProjectName} %{JS: %{IsStatic} ? 'STATIC' : 'SHARED'}
@if '%{Type}' === 'shared'
  %{GlobalHdrFileName}
@endif
  %{SrcFileName}
  %{HdrFileName}
@if %{IsQtPlugin}
  %{PluginJsonFile}
@endif
@if %{HasTranslation}
  ${TS_FILES}
@endif
)
@if '%{QtModule}' != 'none'

target_link_libraries(%{ProjectName} PRIVATE Qt${QT_VERSION_MAJOR}::%{QtModuleUpperCase})
@endif
@if '%{IsShared}'

target_compile_definitions(%{ProjectName} PRIVATE %{LibraryDefine})
@endif
@if %{HasTranslation}

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
@endif
