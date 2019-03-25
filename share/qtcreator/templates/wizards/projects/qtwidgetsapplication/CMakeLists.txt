cmake_minimum_required(VERSION 3.1)

project(%{ProjectName} LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(${PROJECT_NAME}
  "%{MainFileName}"
  "%{SrcFileName}"
  "%{HdrFileName}"
@if %{GenerateForm}
  "%{FormFileName}"
@endif
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets)
