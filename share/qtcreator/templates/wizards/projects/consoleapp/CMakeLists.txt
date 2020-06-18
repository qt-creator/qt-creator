cmake_minimum_required(VERSION 3.14)

project(%{ProjectName} LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

@if %{HasTranslation}
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core LinguistTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core LinguistTools REQUIRED)

set(TS_FILES %{TsFileName})
@else
find_package(Qt5Core)
@endif

add_executable(%{ProjectName}
  %{CppFileName}
@if %{HasTranslation}
  ${TS_FILES}
@endif
)
target_link_libraries(%{ProjectName} Qt${QT_VERSION_MAJOR}::Core)
@if %{HasTranslation}

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
@endif
