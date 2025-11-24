# Created by the Coco Qt Creator plugin. Do not edit!

set(coverage_flags_list
)
list(JOIN coverage_flags_list " " coverage_flags)

foreach(var IN ITEMS CMAKE_C_COMPILER CMAKE_CXX_COMPILER)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "Variable ${var} must be defined.")
    endif()
endforeach()

set(CMAKE_C_FLAGS_INIT "${coverage_flags}"
    CACHE STRING "Coverage flags for the C compiler." FORCE)
set(CMAKE_CXX_FLAGS_INIT "${coverage_flags}"
    CACHE STRING "Coverage flags for the C++ compiler." FORCE)
set(CMAKE_EXE_LINKER_FLAGS_INIT "${coverage_flags}"
    CACHE STRING "Coverage flags for the linker." FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${coverage_flags}"
    CACHE STRING "Coverage flags to link shared libraries." FORCE)
set(CMAKE_STATIC_LINKER_FLAGS_INIT "${coverage_flags}"
    CACHE STRING "Coverage flags to link static libraries." FORCE)

if (DEFINED ENV{SQUISHCOCO})
    cmake_path(SET cocopath $ENV{SQUISHCOCO})
else()
    find_file(cocopath SquishCoco
      PATHS "$ENV{HOME}" /opt/ "/Applications"
      REQUIRED
      NO_DEFAULT_PATH
    )
endif()

if(CMAKE_HOST_APPLE)
    set(wrapperdir "${cocopath}/")
elseif(CMAKE_HOST_UNIX)
    set(wrapperdir "${cocopath}/bin/")
elseif(MINGW)
    set(wrapperdir "${cocopath}/")
else()
    set(wrapperdir "${cocopath}/" )
endif()

get_filename_component(c_compiler "${CMAKE_C_COMPILER}" NAME CACHE)
find_program(code_coverage_c_compiler "cs${c_compiler}"
    PATHS "${wrapperdir}"
    REQUIRED NO_DEFAULT_PATH)
set(CMAKE_C_COMPILER "${code_coverage_c_compiler}"
    CACHE FILEPATH "CoverageScanner wrapper for C compiler" FORCE)

if(WIN32) # Windows cannot handle the filename g++.exe correctly.
    # As a result, we must give the wrapper the profile explicitly as an argument.
    get_filename_component(cxx_compiler_wle "${CMAKE_CXX_COMPILER}" NAME_WLE CACHE)
    string(JOIN " " new_coverage_flags1
        --cs-profile="${wrapperdir}${cxx_compiler_wle}.cspro" "${coverage_flags}")
    string(STRIP "${new_coverage_flags1}" new_coverage_flags)
    set(CMAKE_CXX_FLAGS_INIT "${new_coverage_flags}"
        CACHE STRING "Coverage flags for the C++ compiler." FORCE)
endif()
get_filename_component(cxx_compiler "${CMAKE_CXX_COMPILER}" NAME)
find_program(code_coverage_cxx_compiler "cs${cxx_compiler}"
    PATHS "${wrapperdir}"
    REQUIRED NO_DEFAULT_PATH)
set(CMAKE_CXX_COMPILER "${code_coverage_cxx_compiler}"
    CACHE FILEPATH "CoverageScanner wrapper for C++ compiler" FORCE)

if(${c_compiler} MATCHES "(gcc|.*-gcc|.*-gcc-.*|clang|.*-clang|.*-clang-.*)(\.exe)?")
    # Do nothing, CMake does not use a separate linker program; it uses the compiler.
elseif(DEFINED CMAKE_LINKER)
    get_filename_component(linker_prog "${CMAKE_LINKER}" NAME)
    find_program(code_coverage_linker "cs${linker_prog}"
        PATHS "${wrapperdir}"
        REQUIRED NO_DEFAULT_PATH)
    set(CMAKE_LINKER "${code_coverage_linker}"
        CACHE FILEPATH "CoverageScanner wrapper for linker" FORCE)
elseif(${c_compiler} STREQUAL "cl.exe") # special case for Visual Studio
    find_program(code_coverage_linker "cslink.exe"
        PATHS "${wrapperdir}"
        REQUIRED NO_DEFAULT_PATH)
    set(CMAKE_LINKER "${code_coverage_linker}"
        CACHE FILEPATH "CoverageScanner wrapper for linker" FORCE)
endif()

if(DEFINED CMAKE_AR)
    get_filename_component(ar_prog "${CMAKE_AR}" NAME)
elseif(${c_compiler} MATCHES "(gcc|.*-gcc|.*-gcc-.*|clang|.*-clang|.*-clang-.*)(\.exe)?")
    # In some GCC-like projects, ar is used but CMAKE_AR is not set. So we try to find it directly.
    find_program(ar_prog_path NAMES "ar" "gcc-ar" NO_CACHE)
    get_filename_component(ar_prog "${ar_prog_path}" NAME)
else()
    message(WARNING "The variable CMAKE_AR is not set by CMake. If it is needed, set it with \"-DCMAKE_AR=<path>\".")
endif()
if(DEFINED ar_prog)
    find_program(code_coverage_ar "cs${ar_prog}"
        PATHS "${wrapperdir}"
        REQUIRED NO_DEFAULT_PATH)
    set(CMAKE_AR "${code_coverage_ar}"
        CACHE FILEPATH "CoverageScanner wrapper for ar" FORCE)
endif()

mark_as_advanced(
  cocopath
  code_coverage_c_compiler code_coverage_cxx_compiler code_coverage_linker code_coverage_ar
)

# User-supplied settings follow here:
