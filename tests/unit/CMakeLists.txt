cmake_minimum_required(VERSION 3.10)

if (NOT QT_CREATOR_API_DEFINED)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")

  project(unit)

  enable_testing()

  # Needed for pch
  set(QtCreator_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../")
  option(BUILD_WITH_PCH "Build with precompiled headers" ON)

  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  set(CMAKE_AUTOUIC ON)

  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  set(CMAKE_INCLUDE_CURRENT_DIR ON)

  set(BUILD_EXECUTABLES echo)

  set(IMPLICIT_DEPENDS Qt5::Test)

  include(QtCreatorIDEBranding)
  include(QtCreatorAPI)

  set(WITH_TESTS ON)
  set(GOOGLETEST_DIR ${CMAKE_CURRENT_LIST_DIR}/unittest/3rdparty/googletest)

  find_package(Clang MODULE)
  find_package(Qt5
    COMPONENTS
      Gui Core Core5Compat Widgets Network Qml Concurrent Test Xml MODULE)
  find_package(Threads)
endif()

add_subdirectory(echoserver)
add_subdirectory(unittest)
