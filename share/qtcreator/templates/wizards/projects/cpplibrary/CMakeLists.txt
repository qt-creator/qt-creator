cmake_minimum_required(VERSION 3.1)

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

find_package(Qt5 COMPONENTS %{QtModuleUpperCase} REQUIRED)
@endif

add_library(${PROJECT_NAME} %{JS: %{IsStatic} ? 'STATIC' : 'SHARED'}
@if '%{Type}' === 'shared'
  "%{GlobalHdrFileName}"
@endif
  "%{SrcFileName}"
  "%{HdrFileName}"
@if %{IsQtPlugin}
  "%{PluginJsonFile}"
@endif
)
@if '%{QtModule}' != 'none'

target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::%{QtModuleUpperCase})
@endif
@if '%{IsShared}'

target_compile_definitions(${PROJECT_NAME} PRIVATE %{LibraryDefine})
@endif
