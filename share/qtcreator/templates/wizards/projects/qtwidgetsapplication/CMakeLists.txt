cmake_minimum_required(VERSION 3.5)

project(%{ProjectName} LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

@if %{HasTranslation}
find_package(Qt5 COMPONENTS Widgets LinguistTools REQUIRED)

set(TS_FILES %{TsFileName})
@else
find_package(Qt5 COMPONENTS Widgets REQUIRED)
@endif

add_executable(%{ProjectName}
  %{MainFileName}
  %{SrcFileName}
  %{HdrFileName}
@if %{GenerateForm}
  %{FormFileName}
@endif
@if %{HasTranslation}
  ${TS_FILES}
@endif
)

target_link_libraries(%{ProjectName} PRIVATE Qt5::Widgets)
@if %{HasTranslation}

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
@endif
