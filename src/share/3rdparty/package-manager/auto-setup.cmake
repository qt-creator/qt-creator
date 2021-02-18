if (EXISTS "${CMAKE_SOURCE_DIR}/QtCreatorPackageManager.cmake")
  include("${CMAKE_SOURCE_DIR}/QtCreatorPackageManager.cmake")
endif()

if (QT_CREATOR_SKIP_PACKAGE_MANAGER_SETUP)
  return()
endif()

#
# conan
#

foreach(file conanfile.txt conanfile.py)
  if (EXISTS "${CMAKE_SOURCE_DIR}/${file}")
    set(conanfile_txt "${CMAKE_SOURCE_DIR}/${file}")
    break()
  endif()
endforeach()

if (conanfile_txt AND NOT QT_CREATOR_SKIP_CONAN_SETUP)

  set(conanfile_timestamp_file "${CMAKE_BINARY_DIR}/conan-dependencies/conanfile.timestamp")
  file(TIMESTAMP "${conanfile_txt}" conanfile_timestamp)

  set(do_conan_installation ON)
  if (EXISTS "${conanfile_timestamp_file}")
    file(READ "${conanfile_timestamp_file}" old_conanfile_timestamp)
    if ("${conanfile_timestamp}" STREQUAL "${old_conanfile_timestamp}")
      set(do_conan_installation OFF)
    endif()
  endif()

  if (do_conan_installation)
    file(WRITE "${CMAKE_BINARY_DIR}/conan-dependencies/toolchain.cmake" "
      set(CMAKE_C_COMPILER \"${CMAKE_C_COMPILER}\")
      set(CMAKE_CXX_COMPILER \"${CMAKE_CXX_COMPILER}\")
      ")
    if (CMAKE_TOOLCHAIN_FILE)
      file(APPEND "${CMAKE_BINARY_DIR}/conan-dependencies/toolchain.cmake"
        "include(\"${CMAKE_TOOLCHAIN_FILE}\")\n")
    endif()

    file(WRITE "${CMAKE_BINARY_DIR}/conan-dependencies/CMakeLists.txt" "
      cmake_minimum_required(VERSION 3.15)
      project(conan-setup)
      include(\"${CMAKE_CURRENT_LIST_DIR}/conan.cmake\")
      conan_cmake_run(
        CONANFILE \"${conanfile_txt}\"
        INSTALL_FOLDER \"${CMAKE_BINARY_DIR}/conan-dependencies\"
        GENERATORS cmake_paths json
        BUILD missing
        ENV CONAN_CMAKE_TOOLCHAIN_FILE=\"${CMAKE_BINARY_DIR}/conan-dependencies/toolchain.cmake\"
      )")

    execute_process(COMMAND ${CMAKE_COMMAND}
      -S "${CMAKE_BINARY_DIR}/conan-dependencies/"
      -B "${CMAKE_BINARY_DIR}/conan-dependencies/build"
      -G ${CMAKE_GENERATOR}
      -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      RESULT_VARIABLE result
    )
    if (result EQUAL 0)
      file(WRITE "${conanfile_timestamp_file}" "${conanfile_timestamp}")
    endif()
  endif()

  include("${CMAKE_BINARY_DIR}/conan-dependencies/conan_paths.cmake")
endif()
unset(conanfile_txt)

#
# vcpkg
#

if (EXISTS "${CMAKE_SOURCE_DIR}/vcpkg.json" AND NOT QT_CREATOR_SKIP_VCPKG_SETUP)
  find_program(vcpkg_program vcpkg)
  if (NOT vcpkg_program)
    return()
  endif()
  get_filename_component(vpkg_root ${vcpkg_program} DIRECTORY)

  if (NOT EXISTS "${CMAKE_BINARY_DIR}/vcpkg-dependencies/toolchain.cmake")
    file(WRITE "${CMAKE_BINARY_DIR}/vcpkg-dependencies/toolchain.cmake" "
      set(CMAKE_C_COMPILER \"${CMAKE_C_COMPILER}\")
      set(CMAKE_CXX_COMPILER \"${CMAKE_CXX_COMPILER}\")
      ")
    if (CMAKE_TOOLCHAIN_FILE AND NOT
        CMAKE_TOOLCHAIN_FILE STREQUAL "${CMAKE_BINARY_DIR}/vcpkg-dependencies/toolchain.cmake")
      file(APPEND "${CMAKE_BINARY_DIR}/vcpkg-dependencies/toolchain.cmake"
        "include(\"${CMAKE_TOOLCHAIN_FILE}\")\n")
    endif()

    if (VCPKG_TARGET_TRIPLET)
      set(vcpkg_triplet ${VCPKG_TARGET_TRIPLET})
    else()
      if (WIN32)
        set(vcpkg_triplet x64-mingw-static)
        if (CMAKE_CXX_COMPILER MATCHES "cl.exe")
          set(vcpkg_triplet x64-windows)
        endif()
      elseif(APPLE)
        set(vcpkg_triplet x64-osx)
      else()
        set(vcpkg_triplet x64-linux)
      endif()
    endif()

    file(APPEND "${CMAKE_BINARY_DIR}/vcpkg-dependencies/toolchain.cmake" "
      set(VCPKG_TARGET_TRIPLET ${vcpkg_triplet})
      include(\"${vpkg_root}/scripts/buildsystems/vcpkg.cmake\")
    ")
  endif()

  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_BINARY_DIR}/vcpkg-dependencies/toolchain.cmake" CACHE PATH "" FORCE)
endif()
