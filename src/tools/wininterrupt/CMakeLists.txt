cmake_minimum_required(VERSION 3.20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake")

project(wininterrupt)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT QT_CREATOR_API_DEFINED)
  # standalone build
  include(QtCreatorIDEBranding)
  include(QtCreatorAPI)
  qtc_handle_compiler_cache_support()

  # Compile for x86, x64 and arm64
  if (NOT ${PROJECT_NAME}-MultiBuild AND NOT MINGW)
    include(ExternalProject)

    set(generator "Visual Studio 16 2019")
    if(CMAKE_CXX_COMPILER MATCHES "Microsoft Visual Studio/2022/")
      set(generator "Visual Studio 17 2022")
    endif()

    macro (setup_executable arch)
      ExternalProject_Add(${arch}-bld
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        CMAKE_GENERATOR "${generator}"
        CMAKE_GENERATOR_PLATFORM "${arch}"
        CMAKE_ARGS
          -D${PROJECT_NAME}-MultiBuild=ON
        BUILD_COMMAND
          ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE}
        INSTALL_COMMAND
          ${CMAKE_COMMAND} --install . --config ${CMAKE_BUILD_TYPE}
                           --prefix "${CMAKE_BINARY_DIR}" --component wininterrupt
      )
    endmacro()

    if (NOT WININTERRUPT_BUILD_ARCHS)
      set(WININTERRUPT_BUILD_ARCHS arm64 win32 x64)
    endif()
    foreach(arch IN LISTS WININTERRUPT_BUILD_ARCHS)
      setup_executable(${arch})
    endforeach()

    list(LENGTH WININTERRUPT_BUILD_ARCHS build_archs_length)
    if (build_archs_length GREATER 0)
      install(
        DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        DESTINATION .
        COMPONENT wininterrupt
      )
    endif()

    return()
  endif()
endif()

if (NOT WIN32)
  return()
endif()

set(Arch 32)
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(Arch 64)
endif()

if (MSVC_CXX_ARCHITECTURE_ID MATCHES "^ARM")
  set(Arch "arm${Arch}")
endif()

add_qtc_executable(win${Arch}interrupt
  COMPONENT wininterrupt
  SOURCES wininterrupt.c
)
