#
# Internal Qt Creator variable reference
#
foreach(qtcreator_var
    QT_QMAKE_EXECUTABLE CMAKE_PREFIX_PATH CMAKE_C_COMPILER CMAKE_CXX_COMPILER
    CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELWITHDEBINFO)
  set(__just_reference_${qtcreator_var} ${${qtcreator_var}})
endforeach()

if (EXISTS "${CMAKE_SOURCE_DIR}/QtCreatorPackageManager.cmake")
  include("${CMAKE_SOURCE_DIR}/QtCreatorPackageManager.cmake")
endif()

if (QT_CREATOR_SKIP_PACKAGE_MANAGER_SETUP)
  return()
endif()
option(QT_CREATOR_SKIP_PACKAGE_MANAGER_SETUP "Skip Qt Creator's package manager auto-setup" OFF)

#
# conan
#
macro(qtc_auto_setup_conan)
  foreach(file conanfile.txt conanfile.py)
    if (EXISTS "${CMAKE_SOURCE_DIR}/${file}")
      set(conanfile_txt "${CMAKE_SOURCE_DIR}/${file}")
      break()
    endif()
  endforeach()

  if (conanfile_txt AND NOT QT_CREATOR_SKIP_CONAN_SETUP)
    option(QT_CREATOR_SKIP_CONAN_SETUP "Skip Qt Creator's conan package manager auto-setup" OFF)
    set(QT_CREATOR_CONAN_BUILD_POLICY "missing" CACHE STRING "Qt Creator's conan package manager auto-setup build policy. This is used for the BUILD property of cmake_conan_run")

    # Get conan from Qt SDK
    set(qt_creator_ini "${CMAKE_CURRENT_LIST_DIR}/../QtProject/QtCreator.ini")
    file(STRINGS ${qt_creator_ini} install_settings REGEX "^InstallSettings=.*$")
    if (install_settings)
      string(REPLACE "InstallSettings=" "" install_settings "${install_settings}")
      set(qt_creator_ini "${install_settings}/QtProject/QtCreator.ini")
      file(TO_CMAKE_PATH "${qt_creator_ini}" qt_creator_ini)
    endif()

    file(STRINGS ${qt_creator_ini} conan_executable REGEX "^ConanFilePath=.*$")
    if (conan_executable)
      string(REPLACE "ConanFilePath=" "" conan_executable "${conan_executable}")
      file(TO_CMAKE_PATH "${conan_executable}" conan_executable)
      get_filename_component(conan_path "${conan_executable}" DIRECTORY)
    endif()

    set(path_sepparator ":")
    if (WIN32)
      set(path_sepparator ";")
    endif()
    if (conan_path)
      set(ENV{PATH} "${conan_path}${path_sepparator}$ENV{PATH}")
    endif()

    find_program(conan_program conan)
    if (NOT conan_program)
      message(WARNING "Qt Creator: conan executable not found. "
                      "Package manager auto-setup will be skipped. "
                      "To disable this warning set QT_CREATOR_SKIP_CONAN_SETUP to ON.")
      return()
    endif()

    set(conanfile_timestamp_file "${CMAKE_BINARY_DIR}/conan-dependencies/conanfile.timestamp")
    file(TIMESTAMP "${conanfile_txt}" conanfile_timestamp)

    set(do_conan_installation ON)
    if (EXISTS "${conanfile_timestamp_file}")
      file(READ "${conanfile_timestamp_file}" old_conanfile_timestamp)
      if ("${conanfile_timestamp}" STREQUAL "${old_conanfile_timestamp}")
        set(do_conan_installation OFF)
      endif()
    endif()

    set(conanfile_build_policy_file "${CMAKE_BINARY_DIR}/conan-dependencies/conanfile.buildpolicy")
    if (EXISTS "${conanfile_build_policy_file}")
      file(READ "${conanfile_build_policy_file}" build_policy)
      if (NOT "${build_policy}" STREQUAL "${QT_CREATOR_CONAN_BUILD_POLICY}")
        set(do_conan_installation ON)
      endif()
    endif()

    if (do_conan_installation)
      message(STATUS "Qt Creator: conan package manager auto-setup. "
                     "Skip this step by setting QT_CREATOR_SKIP_CONAN_SETUP to ON.")
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
          BUILD ${QT_CREATOR_CONAN_BUILD_POLICY}
          ENV CONAN_CMAKE_TOOLCHAIN_FILE=\"${CMAKE_BINARY_DIR}/conan-dependencies/toolchain.cmake\"
        )")

      execute_process(COMMAND ${CMAKE_COMMAND}
        -S "${CMAKE_BINARY_DIR}/conan-dependencies/"
        -B "${CMAKE_BINARY_DIR}/conan-dependencies/build"
        -C "${CMAKE_BINARY_DIR}/qtcsettings.cmake"
        -D "CMAKE_TOOLCHAIN_FILE=${CMAKE_BINARY_DIR}/conan-dependencies/toolchain.cmake"
        -G ${CMAKE_GENERATOR}
        -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        RESULT_VARIABLE result
      )
      if (result EQUAL 0)
        file(WRITE "${conanfile_timestamp_file}" "${conanfile_timestamp}")
        file(WRITE "${conanfile_build_policy_file}" ${QT_CREATOR_CONAN_BUILD_POLICY})
      else()
        message(WARNING "Qt Creator's conan package manager auto-setup failed. Consider setting "
                        "QT_CREATOR_SKIP_CONAN_SETUP to ON and reconfigure to skip this step.")
        return()
      endif()
    endif()

    include("${CMAKE_BINARY_DIR}/conan-dependencies/conan_paths.cmake")
  endif()
  unset(conanfile_txt)
endmacro()
qtc_auto_setup_conan()

#
# vcpkg
#
macro(qtc_auto_setup_vcpkg)
  if (EXISTS "${CMAKE_SOURCE_DIR}/vcpkg.json" AND NOT QT_CREATOR_SKIP_VCPKG_SETUP)
    option(QT_CREATOR_SKIP_VCPKG_SETUP "Skip Qt Creator's vcpkg package manager auto-setup" OFF)

    find_program(vcpkg_program vcpkg)
    if (NOT vcpkg_program)
      message(WARNING "Qt Creator: vcpkg executable not found. "
                      "Package manager auto-setup will be skipped. "
                      "To disable this warning set QT_CREATOR_SKIP_VCPKG_SETUP to ON.")
      return()
    endif()
    get_filename_component(vpkg_root ${vcpkg_program} DIRECTORY)

    if (NOT EXISTS "${CMAKE_BINARY_DIR}/vcpkg-dependencies/toolchain.cmake")
      message(STATUS "Qt Creator: vcpkg package manager auto-setup. "
                     "Skip this step by setting QT_CREATOR_SKIP_VCPKG_SETUP to ON.")

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
endmacro()
qtc_auto_setup_vcpkg()
