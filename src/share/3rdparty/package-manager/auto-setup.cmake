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

    find_program(conan_program conan)
    if (NOT conan_program)
      message(WARNING "Qt Creator: conan executable not found. "
                      "Package manager auto-setup will be skipped. "
                      "To disable this warning set QT_CREATOR_SKIP_CONAN_SETUP to ON.")
      return()
    endif()
    execute_process(COMMAND ${conan_program} --version
      RESULT_VARIABLE result_code
      OUTPUT_VARIABLE conan_version_output
      ERROR_VARIABLE conan_version_output)
    if (NOT result_code EQUAL 0)
      message(FATAL_ERROR "conan --version failed='${result_code}: ${conan_version_output}")
    endif()

    string(REGEX REPLACE ".*Conan version ([0-9].[0-9]).*" "\\1" conan_version "${conan_version_output}")

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

      file(COPY "${conanfile_txt}" DESTINATION "${CMAKE_BINARY_DIR}/conan-dependencies/")

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

          unset(CMAKE_PROJECT_INCLUDE_BEFORE CACHE)
          project(conan-setup)

          if (${conan_version} VERSION_GREATER_EQUAL 2.0)
            include(\"${CMAKE_CURRENT_LIST_DIR}/conan_support.cmake\")
            conan_profile_detect_default()
            detect_host_profile(\"${CMAKE_BINARY_DIR}/conan-dependencies/conan_host_profile\")

            conan_install(
              -pr \"${CMAKE_BINARY_DIR}/conan-dependencies/conan_host_profile\"
              --build=${QT_CREATOR_CONAN_BUILD_POLICY}
              -s build_type=${CMAKE_BUILD_TYPE}
              -g CMakeDeps)
            if (CONAN_INSTALL_SUCCESS)
              file(WRITE \"${CMAKE_BINARY_DIR}/conan-dependencies/conan_paths.cmake\" \"
                list(PREPEND CMAKE_PREFIX_PATH \\\"\${CONAN_GENERATORS_FOLDER}\\\")
                list(PREPEND CMAKE_MODULE_PATH \\\"\${CONAN_GENERATORS_FOLDER}\\\")
              \")
            endif()
          else()
            include(\"${CMAKE_CURRENT_LIST_DIR}/conan.cmake\")
            conan_cmake_run(
              CONANFILE \"${conanfile_txt}\"
              INSTALL_FOLDER \"${CMAKE_BINARY_DIR}/conan-dependencies\"
              GENERATORS cmake_paths cmake_find_package json
              BUILD ${QT_CREATOR_CONAN_BUILD_POLICY}
              ENV CONAN_CMAKE_TOOLCHAIN_FILE=\"${CMAKE_BINARY_DIR}/conan-dependencies/toolchain.cmake\"
            )
          endif()
        ")

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
    execute_process(COMMAND ${vcpkg_program} version
      RESULT_VARIABLE result_code
      OUTPUT_VARIABLE vcpkg_version_output
      ERROR_VARIABLE vcpkg_version_output)
    if (NOT result_code EQUAL 0)
      message(FATAL_ERROR "vcpkg version failed='${result_code}: ${vcpkg_version_output}")
    endif()

    # Resolve any symlinks
    get_filename_component(vpkg_program_real_path ${vcpkg_program} REALPATH)
    get_filename_component(vpkg_root ${vpkg_program_real_path} DIRECTORY)

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
          if (CMAKE_CXX_COMPILER MATCHES ".*/(.*)/cl.exe")
            set(vcpkg_triplet ${CMAKE_MATCH_1}-windows)
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
