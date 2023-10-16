cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake")

project(qtcreatorcdbext)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT QT_CREATOR_API_DEFINED)
  # standalone build
  include(QtCreatorIDEBranding)
  include(QtCreatorAPI)
  qtc_handle_compiler_cache_support()
endif()

if (NOT WIN32 OR NOT MSVC)
  return()
endif()

find_library(DbgEngLib dbgeng)

set(ArchSuffix "32")
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(ArchSuffix "64")
endif()

if (MSVC_CXX_ARCHITECTURE_ID MATCHES "^ARM")
  set(ArchSuffix "arm${ArchSuffix}")
endif()

add_qtc_library(qtcreatorcdbext SHARED
  COMPONENT qtcreatorcdbext
  DEPENDS ${DbgEngLib}
  DESTINATION lib/qtcreatorcdbext${ArchSuffix}/
  SOURCES
    common.cpp common.h
    containers.cpp containers.h
    eventcallback.cpp eventcallback.h
    extensioncontext.cpp extensioncontext.h
    gdbmihelpers.cpp gdbmihelpers.h
    iinterfacepointer.h
    knowntype.h
    outputcallback.cpp outputcallback.h
    qtcreatorcdbext.def
    qtcreatorcdbextension.cpp
    stringutils.cpp stringutils.h
    symbolgroup.cpp symbolgroup.h
    symbolgroupnode.cpp symbolgroupnode.h
    symbolgroupvalue.cpp symbolgroupvalue.h
)

qtc_library_enabled(_library_enabled qtcreatorcdbext)
if (_library_enabled)
  # statically link MSVC runtime
  set_property(TARGET qtcreatorcdbext PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  target_compile_options(qtcreatorcdbext PUBLIC /EHsc)

  find_package(Python3 3.8 COMPONENTS Development)

  if (NOT ${Python3_Development_FOUND})
    message(WARNING "PythonLibs (at least version 3.8) not found. qtcreatorcdbext will be built without Python support.")
    return()
  endif()

  set(PythonRegex "^(.*)/(.*)/(python([0-9]+))${CMAKE_IMPORT_LIBRARY_SUFFIX}$")
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(PythonRegex "^(.*)/(.*)/(python([0-9]+)_d)${CMAKE_IMPORT_LIBRARY_SUFFIX}$")
  endif()

  foreach(lib IN LISTS Python3_LIBRARIES)
    if (lib MATCHES ${PythonRegex})
      if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(PythonZipFileName "python${CMAKE_MATCH_4}_d.zip")
      else()
        set(PythonZipFileName "python${CMAKE_MATCH_4}.zip")
      endif()
      set(PythonNameWithVersion "${CMAKE_MATCH_3}")

      set(PythonDll "${CMAKE_MATCH_1}/${PythonNameWithVersion}${CMAKE_SHARED_LIBRARY_SUFFIX}")
      set(PythonExe "${CMAKE_MATCH_1}/python${CMAKE_EXECUTABLE_SUFFIX}")
      set(PythonZip "${CMAKE_MATCH_1}/${PythonZipFileName}")

      break()
    endif()
  endforeach()

  if (NOT PythonDll)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
      message(WARNING "The Debug build of Qt Creator requires Debug Python libraries. Please check your Python installation")
    endif()
    message(WARNING "PythonDll not found. qtcreatorcdbext will be built without Python support.")
    return()
  endif()

  # Support for cross-compilation for arm64 on a x64 system
  if (MSVC_CXX_ARCHITECTURE_ID MATCHES "^ARM" AND CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^AMD")
    find_program(dumpbin_executable dumpbin)
    find_program(lib_executable lib)

    string(TOLOWER ${MSVC_CXX_ARCHITECTURE_ID} lower_arch_name)

    if (NOT dumpbin_executable OR NOT lib_executable)
      message(WARNING "Couldn't locate dumpbin.exe or lib.exe executables")
      return()
    endif()

    if (Python3_VERSION VERSION_LESS "3.11.0")
      message(WARNING "Python 3.11.0 needs to be installed. This version is the first version that has arm64 Windows support")
      return()
    endif()

    execute_process(
      COMMAND ${dumpbin_executable} /exports ${PythonDll}
      OUTPUT_VARIABLE dumpbin_output
      RESULT_VARIABLE dumpbin_result)

    string(REGEX REPLACE ".*[ \t]+ordinal[ \t]+hint[ \t]+RVA[ \t]+name[\r\n][\r\n]" "" dumpbin_output "${dumpbin_output}")
    string(REGEX REPLACE "[\r\n][ \t]+Summary[\r\n].*" "" dumpbin_output "${dumpbin_output}")
    string(REGEX REPLACE "([ \t]+[0-9]+)([ \t]+[a-fA-F0-9]+)([ \t]+[a-fA-F0-9]+)[ \t]+([a-zA-Z0-9_]+)( = [a-zA-Z0-9_]+[\r\n]|[\r\n])" "\\4;" filter_output "${dumpbin_output}")

    string(APPEND pythondef "LIBRARY ${PythonNameWithVersion}\nEXPORTS\n")
    foreach(var IN LISTS filter_output)
      if (var)
        string(APPEND pythondef "${var}\n")
      endif()
    endforeach()
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${PythonNameWithVersion}.def "${pythondef}")

    execute_process(
      COMMAND ${lib_executable}
              /def:${CMAKE_CURRENT_BINARY_DIR}/${PythonNameWithVersion}.def
              /out:${CMAKE_CURRENT_BINARY_DIR}/${PythonNameWithVersion}.lib /machine:${lower_arch_name} /nologo)
    set(Python3_LIBRARIES "${CMAKE_CURRENT_BINARY_DIR}/${PythonNameWithVersion}.lib")

    if (NOT PythonTargetArchDll AND ENV{PythonTargetArchDll})
      set(PythonTargetArchDll $ENV{PythonTargetArchDll})
    endif()

    if (NOT PythonTargetArchDll)
      set(python_embed_url "https://www.python.org/ftp/python/${Python3_VERSION}/python-${Python3_VERSION}-embed-${lower_arch_name}.zip")
      message(STATUS "Downloading ${python_embed_url}")

      foreach(retry RANGE 10)
        file(DOWNLOAD ${python_embed_url} ${CMAKE_CURRENT_BINARY_DIR}/python-embed.zip)
        file(SIZE ${CMAKE_CURRENT_BINARY_DIR}/python-embed.zip fileSize)
        if (fileSize GREATER 0)
          break()
        endif()
      endforeach()

      file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/python-embed)
      file(ARCHIVE_EXTRACT INPUT ${CMAKE_CURRENT_BINARY_DIR}/python-embed.zip DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/python-embed)

      set(PythonTargetArchDll ${CMAKE_CURRENT_BINARY_DIR}/python-embed/${PythonNameWithVersion}${CMAKE_SHARED_LIBRARY_SUFFIX})
    endif()

    if (NOT PythonTargetArchDll)
      message(WARNING "PythonTargetArchDll CMake parameter or ENV{PythonTargetArchDll} was not configured and the Python runtime cannot be configured")
      return()
    endif()

    set(PythonDll "${PythonTargetArchDll}")
  endif()

  extend_qtc_library(qtcreatorcdbext
    DEPENDS "${Python3_LIBRARIES}"
    INCLUDES "${Python3_INCLUDE_DIRS}"
    DEFINES WITH_PYTHON=1 PY_SSIZE_T_CLEAN
    SOURCES
      pycdbextmodule.cpp pycdbextmodule.h
      pyfield.cpp pyfield.h
      pystdoutredirect.cpp pystdoutredirect.h
      pytype.cpp pytype.h
      pyvalue.cpp pyvalue.h
  )

  if (NOT EXISTS "${PythonZip}" AND
      NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${PythonZipFileName}")
    include(CreatePythonXY)
    create_python_xy("${PythonExe}" "${CMAKE_CURRENT_BINARY_DIR}/${PythonZipFileName}")
  endif()

  if (NOT EXISTS "${PythonZip}" AND
      EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${PythonZipFileName}")
    set(PythonZip "${CMAKE_CURRENT_BINARY_DIR}/${PythonZipFileName}")
  endif()

  list(APPEND deployPythonFiles "${PythonDll}")
  list(APPEND deployPythonFiles "${PythonZip}")

  install(FILES ${deployPythonFiles}
          DESTINATION lib/qtcreatorcdbext${ArchSuffix}/
          COMPONENT qtcreatorcdbext)

  add_custom_target(copy_python_dll ALL VERBATIM)

  qtc_output_binary_dir(output_binary_dir)
  add_custom_command(TARGET copy_python_dll POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${deployPythonFiles} "${output_binary_dir}/lib/qtcreatorcdbext${ArchSuffix}/"
    VERBATIM
  )
endif()
