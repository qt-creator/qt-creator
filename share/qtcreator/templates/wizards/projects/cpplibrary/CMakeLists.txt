cmake_minimum_required(VERSION 3.14)

project(%{ProjectName} LANGUAGES CXX)

@if '%{QtModule}' != 'none'
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
@endif
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
@if '%{QtModule}' != 'none'

@if %{HasTranslation}
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS %{QtModuleUpperCase} LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS %{QtModuleUpperCase} LinguistTools)

set(TS_FILES %{TsFileName})
@else
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS %{QtModuleUpperCase})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS %{QtModuleUpperCase})
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

if(COMMAND qt_create_translation)
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()
@endif
