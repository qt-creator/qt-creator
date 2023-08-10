cmake_minimum_required(VERSION 3.16)

project(simple_test_app LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WINDOWS)
  set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Network Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Network Widgets)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Script Webkit WebkitWidgets QUIET)
find_package(Boost QUIET)
find_package(Eigen2 QUIET)
find_package(Eigen3 QUIET)

if (${QT_VERSION_MAJOR} LESS 6)
  find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Xml)
else()
  find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core5Compat)
endif()

add_executable(simple_test_app simple_test_app.cpp)

add_compile_definitions(simple_test_app "USE_GUILIB")

if (Qt${QT_VERSION_MAJOR}Script_FOUND)
  add_compile_definitions(simple_test_app "HAS_SCRIPT")
endif()

if (Boost_FOUND)
  add_compile_definitions(simple_test_app "HAS_BOOST")
  target_include_directories(simple_test_app PRIVATE ${Boost_INCLUDE_DIRS})
endif()

if (EIGEN2_FOUND)
  add_compile_definitions(simple_test_app "HAS_EIGEN2")
  target_include_directories(simple_test_app PRIVATE ${EIGEN2_INCLUDE_DIR})
endif()

if (EIGEN3_FOUND)
  add_compile_definitions(simple_test_app "HAS_EIGEN3")
  target_include_directories(simple_test_app PRIVATE ${EIGEN3_INCLUDE_DIR})
endif()

target_link_libraries(simple_test_app PRIVATE Qt${QT_VERSION_MAJOR}::Core
                                      PRIVATE Qt${QT_VERSION_MAJOR}::CorePrivate
                                      PRIVATE Qt${QT_VERSION_MAJOR}::Gui
                                      PRIVATE Qt${QT_VERSION_MAJOR}::Network
                                      PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
if (${QT_VERSION_MAJOR} LESS 6)
  target_link_libraries(simple_test_app PRIVATE Qt${QT_VERSION_MAJOR}::Xml)
  if (Qt${QT_VERSION_MAJOR}Script_FOUND)
    target_link_libraries(simple_test_app PRIVATE Qt${QT_VERSION_MAJOR}::Script)
  endif()
else()
  target_link_libraries(simple_test_app PRIVATE Qt${QT_VERSION_MAJOR}::Core5Compat)
endif()

if (Qt${QT_VERSION_MAJOR}Webkit_FOUND)
  target_link_libraries(simple_test_app PRIVATE Qt${QT_VERSION_MAJOR}::Webkit)
endif()

if (Qt${QT_VERSION_MAJOR}WebkitWidgets_FOUND)
  target_link_libraries(simple_test_app PRIVATE Qt${QT_VERSION_MAJOR}::WebkitWidgets)
endif()

# and the plugin
add_library(simple_test_plugin SHARED simple_test_plugin.cpp)
target_link_libraries(simple_test_plugin PRIVATE Qt${QT_VERSION_MAJOR}::Core)
