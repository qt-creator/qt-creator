cmake_minimum_required(VERSION 3.20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake")

project(wininterrupt)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(Arch 32)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(Arch 64)
endif()

if(MSVC_CXX_ARCHITECTURE_ID MATCHES "^ARM")
  set(Arch "arm${Arch}")
endif()

if (NOT QT_CREATOR_API_DEFINED)
  # standalone build
  set(QTC_SBOM_READY ON)
  include(QtCreatorIDEBranding)
  include(QtCreatorAPI)
  include(QtCreatorSbom)
  qtc_handle_compiler_cache_support()

  # Need to look for Qt6, so that we can use the SBOM feature, even though the
  # project doesn't link to Qt.
  # Explicitly disable the version checks, because when configuring for an architecture that
  # is not compatible with the found Qt, we'd fail to find the package. We don't care about
  # binary compatibility here, we only want the SBOM feature.
  if(QT_GENERATE_SBOM)
    set(QT_NO_PACKAGE_VERSION_CHECK ON)
    set(QT_NO_PACKAGE_VERSION_INCOMPATIBLE_WARNING ON)
    find_package(Qt6 REQUIRED)
    unset(QT_NO_PACKAGE_VERSION_CHECK)
    unset(QT_NO_PACKAGE_VERSION_INCOMPATIBLE_WARNING)
  endif()

  set(QT_WIN_INTERRUPT_STANDALONE_BUILD TRUE)

  # Compile for x86, x64 and arm64
  if (NOT ${PROJECT_NAME}-MultiBuild AND NOT MINGW)
    include(ExternalProject)

    set(generator "Visual Studio 16 2019")
    if(CMAKE_CXX_COMPILER MATCHES "Microsoft Visual Studio/2022/")
      set(generator "Visual Studio 17 2022")
    endif()

    string(REPLACE ";" "|" CMAKE_PREFIX_PATH_ALT_SEP "${CMAKE_PREFIX_PATH}")

    set(extra_cmake_args "")
    set(extra_install_commands "")
    if(QT_GENERATE_SBOM)
      list(APPEND extra_cmake_args -DQT_GENERATE_SBOM=${QT_GENERATE_SBOM})
      list(APPEND extra_install_commands
        && ${CMAKE_COMMAND} --install . --config ${CMAKE_BUILD_TYPE}
                            --prefix "${CMAKE_BINARY_DIR}" --component sbom
      )
    endif()

    macro (setup_executable arch)
      ExternalProject_Add(${arch}-bld
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        CMAKE_GENERATOR "${generator}"
        CMAKE_GENERATOR_PLATFORM "${arch}"
        LIST_SEPARATOR |
        USES_TERMINAL_INSTALL TRUE
        CMAKE_ARGS
          -D${PROJECT_NAME}-MultiBuild=ON
          -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_ALT_SEP}
          -DCMAKE_CONFIGURATION_TYPES=${CMAKE_BUILD_TYPE}
          ${extra_cmake_args}
        BUILD_COMMAND
          ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE}
        INSTALL_COMMAND
          ${CMAKE_COMMAND} --install . --config ${CMAKE_BUILD_TYPE}
                           --prefix "${CMAKE_BINARY_DIR}" --component wininterrupt
          ${extra_install_commands}
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
      if(QT_GENERATE_SBOM)
        qtc_sbom_get_sbom_install_path(sbom_install_path)
        # Because we are installing a dir, the destination should be the same without the dir
        # name.
        get_filename_component(sbom_install_dest "${sbom_install_path}" DIRECTORY)
        install(
          DIRECTORY "${CMAKE_BINARY_DIR}/${sbom_install_path}"
          DESTINATION "${sbom_install_dest}"
          COMPONENT wininterrupt
        )
      endif()
    endif()

    return()
  endif()
endif()

if (NOT WIN32)
  return()
endif()

if(QT_WIN_INTERRUPT_STANDALONE_BUILD AND QT_GENERATE_SBOM)
  qtc_setup_sbom()
  qtc_sbom_compute_cpe(project_cpe
      VENDOR "qt"
      PRODUCT "${PROJECT_NAME}"
      VERSION "${IDE_VERSION}"
  )
  set(QT_SBOM_LICENSE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../LICENSES")
  qtc_sbom_begin_project(
    SBOM_PROJECT_NAME "${PROJECT_NAME}-${Arch}"
    QT_REPO_PROJECT_NAME "${PROJECT_NAME}-${Arch}"
    PURL_NAMESPACE "qt"
    PURL_NAME "${PROJECT_NAME}"
    CPE "${project_cpe}"
  )
endif()

add_qtc_executable(win${Arch}interrupt
  COMPONENT wininterrupt
  SOURCES wininterrupt.c
)

if(QT_WIN_INTERRUPT_STANDALONE_BUILD AND QT_GENERATE_SBOM)
  qtc_sbom_end_project()
endif()
