cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake")

project(wininterrupt)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT QT_CREATOR_API_DEFINED)
  # standalone build
  include(QtCreatorIDEBranding)
  include(QtCreatorAPI)
endif()

if (NOT WIN32)
  return()
endif()

set(Arch 32)
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(Arch 64)
endif()

add_qtc_executable(win${Arch}interrupt
  COMPONENT wininterrupt
  SOURCES wininterrupt.c
)
