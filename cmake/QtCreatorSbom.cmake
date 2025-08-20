# Sets up SBOM support when the Qt version is 6.8.3+.
# SBOM generation can be enabled by setting QT_GENERATE_SBOM to ON.
# By default generation is OFF.
function(qtc_setup_sbom)
  get_cmake_property(setup_called _qtc_internal_sbom_setup_called)
  if(setup_called)
    return()
  endif()

  set_property(GLOBAL PROPERTY _qtc_internal_sbom_setup_called TRUE)

  # Just-in-case opt-in to stop running any kind of SBOM code.
  if(QTC_NO_GENERATE_SBOM)
    qtc_force_disable_sbom()
    return()
  endif()

  # Check that the Qt version supports generating sboms.
  if(Qt6_VERSION VERSION_LESS 6.8.3)
    set(sbom_supported FALSE)
  else()
    set(sbom_supported TRUE)
  endif()

  # Show error when trying to generate SBOM, but the underlying Qt does not support it.
  if(QT_GENERATE_SBOM AND NOT sbom_supported)

    if(NOT Qt6_DIR)
      message(FATAL_ERROR
        "SBOM generation requested via -DQT_GENERATE_SBOM=ON, but no valid Qt6 version found.")
    endif()

    message(FATAL_ERROR
      "Found Qt version '${Qt6_VERSION}' at '${Qt6_DIR}' does not have all necessary support for "
      "generating an SBOM.")
  endif()

  set(generate_by_default "OFF")
  option(QT_GENERATE_SBOM "Generate SBOM documents in SPDX v2.3 tag:value format."
    ${generate_by_default})

  # Return early if SBOM is not supported and it isn't explicitly enabled.
  if(NOT sbom_supported)
    return()
  endif()

  # Call Qt's internal setup function.
  _qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "${generate_by_default}"
  )

  add_feature_info("Build SBOMs" "${QT_GENERATE_SBOM}" "")
endfunction()

function(qtc_force_disable_sbom)
  set(QT_GENERATE_SBOM "OFF" CACHE BOOL "Generate SBOM documents in SPDX v2.3 tag:value format."
    FORCE)
endfunction()

# For each Qt Creator repo that wants to ship an SBOM, a set(QTC_SBOM_READY ON) assignment
# needs to be added before the find_package(QtCreator) call, to ensure SBOM generation doesn't
# get disabled even if QT_GENERATE_SBOM is ON.
# This assumes the repo is actually SBOM ready, so has calls to qtc_setup_sbom,
# qtc_sbom_begin_project, etc.
# Otherwise SBOM generation is force disabled.
# This prevents CMake errors in calls like add_qtc_library which will try to add SBOM info to a
# target, in a repo that is not SBOM-ready, but has QT_GENERATE_SBOM enabled.
function(qtc_check_if_project_is_sbom_ready)
  if(NOT QTC_SBOM_READY AND QT_GENERATE_SBOM)
    qtc_force_disable_sbom()
    if(NOT QT_HIDE_SBOM_READY_WARNING)
      message(STATUS "QT_GENERATE_SBOM is ON, but the project is not marked as SBOM-ready. "
        "Skipping SBOM generation.\n"
        "Pass -DQT_HIDE_SBOM_READY_WARNING=ON to hide this message.\n"
        "Or add set(QTC_SBOM_READY ON) to the project before find_package(QtCreator) to mark the "
        "project as SBOM-ready.")
    endif()
  endif()
endfunction()

# This function should be called to set up the SBOM generation for a project.
# One project == one document.
# Each sbom-enabled target will have its data written into the document.
# Various options can be passed to set up information like project name, version, download location,
# document install prefix, etc. See the options below.
# The function and its _end() counterpart can be called multiple times inside a CMake project, to
# create multiple documents and split up targets into specific documents.
function(qtc_sbom_begin_project)
  if(NOT QT_GENERATE_SBOM)
    return()
  endif()

  # These are 99% of the options that Qt knows about, excluding some internal ones.
  set(generic_opt_args
      USE_GIT_VERSION
  )
  set(generic_single_args
      INSTALL_PREFIX
      INSTALL_SBOM_DIR
      LICENSE_EXPRESSION
      SUPPLIER
      SUPPLIER_URL
      DOWNLOAD_LOCATION
      DOCUMENT_NAMESPACE
      VERSION
      SBOM_PROJECT_NAME
      QT_REPO_PROJECT_NAME
      CPE
  )
  set(generic_multi_args
      COPYRIGHTS
  )

  # Qt ones + Qt Creator specific options.
  set(no_default_options
    NO_DEFAULT_USE_GIT_VERSION
    NO_DEFAULT_INSTALL_SBOM_DIR
    NO_DEFAULT_SUPPLIER
    NO_DEFAULT_SUPPLIER_URL
    NO_DEFAULT_DOWNLOAD_LOCATION
    NO_DEFAULT_CPE
    NO_DEFAULT_PURL_NAME
    NO_DEFAULT_PURL_NAMESPACE
    NO_DEFAULT_VCS_URL_BASE
    NO_DEFAULT_COPYRIGHTS
    NO_DEFAULT_LICENSE_EXPRESSION
    NO_DEFAULT_VERSION
  )

  set(opt_args
    ${generic_opt_args}
    ${no_default_options}
    NO_DEFAULT_CREATOR_SBOM_OPTIONS
  )
  set(single_args
    PURL_NAME
    PURL_NAMESPACE
    VCS_URL_BASE
    DEFAULT_LICENSE_EXPRESSION
    DEFAULT_VERSION
    ${generic_single_args}
  )
  set(multi_args
    ${generic_multi_args}
    DEFAULT_COPYRIGHTS
  )

  cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
  qtc_validate_all_args_are_parsed(arg)

  set(begin_project_args "")

  if(arg_NO_DEFAULT_CREATOR_SBOM_OPTIONS)
    foreach(option_name IN LISTS no_default_options)
      set(arg_${option_name} TRUE)
    endforeach()
  endif()

  # Set default Qt Creator specific values, unless caller opted into no defaults.
  if(NOT arg_VERSION AND NOT arg_DEFAULT_VERSION AND NOT arg_NO_DEFAULT_USE_GIT_VERSION)
    if(IDE_VERSION)
      # Passed as the package version.
      set(arg_VERSION "${IDE_VERSION}")
      # Passed as each target version.
      set(arg_DEFAULT_VERSION "${IDE_VERSION}")
    else()
      # Rely on version extracted from git.
      set(arg_USE_GIT_VERSION TRUE)
    endif()
  endif()

  if(NOT arg_INSTALL_SBOM_DIR AND NOT arg_NO_DEFAULT_INSTALL_SBOM_DIR)
    qtc_sbom_get_sbom_install_path(arg_INSTALL_SBOM_DIR)
  endif()

  if(NOT arg_SUPPLIER AND NOT arg_NO_DEFAULT_SUPPLIER)
    set(arg_SUPPLIER "TheQtCompany")
  endif()
  if(arg_SUPPLIER)
      set_property(GLOBAL PROPERTY _qtc_sbom_supplier "${arg_SUPPLIER}")
  endif()

  if(NOT arg_SUPPLIER_URL AND NOT arg_NO_DEFAULT_SUPPLIER_URL)
    set(arg_SUPPLIER_URL "https://qt.io")
  endif()

  if(arg_DOWNLOAD_LOCATION)
    set_property(GLOBAL PROPERTY _qtc_sbom_download_location_base "${arg_DOWNLOAD_LOCATION}")
  elseif(NOT arg_NO_DEFAULT_DOWNLOAD_LOCATION)
    qtc_sbom_get_repo_source_download_location_default(OUT_VAR download_location_base)
    set_property(GLOBAL PROPERTY _qtc_sbom_download_location_base "${download_location_base}")
  endif()
  qtc_sbom_get_repo_source_download_location(OUT_VAR download_location_versioned)
  if(download_location_versioned)
    set(arg_DOWNLOAD_LOCATION "${download_location_versioned}")
  endif()

  if(NOT arg_CPE AND NOT arg_NO_DEFAULT_CPE)
    qtc_sbom_compute_cpe(cpe
      VENDOR "qt"
      PRODUCT "qtcreator"
      VERSION "${IDE_VERSION}"
    )
    set(arg_CPE "${cpe}")
  endif()
  if(arg_CPE)
    set_property(GLOBAL PROPERTY _qtc_sbom_cpe "${arg_CPE}")
  endif()

  if(NOT arg_PURL_NAME AND NOT arg_NO_DEFAULT_PURL_NAME)
    set(arg_PURL_NAME "qt-creator")
  endif()
  if(arg_PURL_NAME)
    set_property(GLOBAL PROPERTY _qtc_sbom_purl_name "${arg_PURL_NAME}")
  endif()

  if(NOT arg_PURL_NAMESPACE AND NOT arg_NO_DEFAULT_PURL_NAMESPACE)
    set(arg_PURL_NAMESPACE "qt-creator")
  endif()
  if(arg_PURL_NAMESPACE)
    set_property(GLOBAL PROPERTY _qtc_sbom_purl_namespace "${arg_PURL_NAMESPACE}")
  endif()

  if(NOT arg_VCS_URL_BASE AND NOT arg_NO_DEFAULT_VCS_URL_BASE)
    qtc_sbom_get_qtc_vcs_url_base_default(OUT_VAR vcs_url)
    set(arg_VCS_URL_BASE "${vcs_url}")
  endif()
  if(arg_VCS_URL_BASE)
    set_property(GLOBAL PROPERTY _qtc_sbom_vcs_base_url "${arg_VCS_URL_BASE}")
  endif()

  if(NOT arg_NO_DEFAULT_COPYRIGHTS AND NOT arg_DEFAULT_COPYRIGHTS AND IDE_COPYRIGHT)
    # Passed as the repo package copyright.
    set(arg_COPYRIGHTS "${IDE_COPYRIGHT}")

    # Passed to QtCreator entities.
    set(arg_DEFAULT_COPYRIGHTS "${IDE_COPYRIGHT}")
  endif()
  if(arg_DEFAULT_COPYRIGHTS)
    set_property(GLOBAL PROPERTY _qtc_sbom_default_copyrights "${arg_DEFAULT_COPYRIGHTS}")
  endif()
  if(arg_NO_DEFAULT_COPYRIGHTS)
    set_property(GLOBAL PROPERTY _qtc_sbom_no_default_copyrights "TRUE")
  endif()

  if(arg_DEFAULT_LICENSE_EXPRESSION)
    set_property(GLOBAL PROPERTY _qtc_sbom_default_license_expression
      "${arg_DEFAULT_LICENSE_EXPRESSION}")
  endif()
  if(arg_NO_DEFAULT_LICENSE_EXPRESSION)
    set_property(GLOBAL PROPERTY _qtc_sbom_no_default_license_expression "TRUE")
  endif()

  if(arg_DEFAULT_VERSION)
    set_property(GLOBAL PROPERTY _qtc_sbom_default_version "${arg_DEFAULT_VERSION}")
  endif()
  if(arg_NO_DEFAULT_VERSION)
    set_property(GLOBAL PROPERTY _qtc_sbom_no_default_version "TRUE")
  endif()

  # Forward any set options.
  qtc_forward_function_args(
      FORWARD_APPEND
      FORWARD_PREFIX arg
      FORWARD_OUT_VAR begin_project_args
      FORWARD_OPTIONS
          ${generic_opt_args}
      FORWARD_SINGLE
          ${generic_single_args}
      FORWARD_MULTI
          ${generic_multi_args}
  )

  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSES")
    set(license_dirs "${CMAKE_CURRENT_SOURCE_DIR}/LICENSES")

    # This is read by the call to _qt_internal_sbom_begin_project, so we need to set it in this
    # scope, because there is no extra option to specify that directly.
    list(APPEND QT_SBOM_LICENSE_DIRS "${license_dirs}")
  endif()

  # find_package(QtCreator) via QtCreatorConfig.cmake might have exported an some sbom path location
  # to the Creator SBOM document. Make sure we make it visible for the Qt API.
  qtc_add_exported_sbom_paths_for_current_project()

  # Call the Qt provided SBOM API.
  _qt_internal_sbom_begin_project(${begin_project_args})
endfunction()

# This function should be called after all relevant targets that should be part of the document
# are created. It triggers the actual generation and installation of the SBOM document.
#
# The DEFER_END_PROJECT option can be used to defer the generation to the end of current
# directory scope. It shouldn't be needed usually because it is auto-enabled when the build system
# detects if there are any targets have not been finalized yet.
function(qtc_sbom_end_project)
  if(NOT QT_GENERATE_SBOM)
    return()
  endif()

  set(opt_args
    DEFER_END_PROJECT
  )
  set(single_args "")
  set(multi_args "")
  cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
  qtc_validate_all_args_are_parsed(arg)

  set(need_to_defer FALSE)
  if(arg_DEFER_END_PROJECT)
    set(need_to_defer TRUE)
  endif()

  # Check if there are any targets that still haven't been finalized.
  get_cmake_property(targets_expecting_finalization _qtc_sbom_targets_expecting_finalization)
  if(targets_expecting_finalization)
    foreach(target IN LISTS targets_expecting_finalization)
      get_target_property(is_finalized "${target}" _qtc_is_finalized)
      if(NOT is_finalized)
        set(need_to_defer TRUE)
        break()
      endif()
    endforeach()
  endif()

  # This is to support the case when a subdirectory first has to process the deferred target
  # calls, before the project should be ended.
  if(need_to_defer)
    qtc_defer_call(qtc_sbom_end_project_impl)
  else()
    qtc_sbom_end_project_impl()
  endif()
endfunction()

# Implementation helper for the SBOM document generation.
function(qtc_sbom_end_project_impl)
  if(NOT QT_GENERATE_SBOM)
    return()
  endif()

  _qt_internal_sbom_end_project()

  # Clean up global properties that were set by the previous project.
  set_property(GLOBAL PROPERTY _qtc_sbom_purl_name "")
  set_property(GLOBAL PROPERTY _qtc_sbom_purl_namespace "")
  set_property(GLOBAL PROPERTY _qtc_sbom_vcs_base_url "")
  set_property(GLOBAL PROPERTY _qtc_sbom_cpe "")
  set_property(GLOBAL PROPERTY _qtc_sbom_download_location_base "")
  set_property(GLOBAL PROPERTY _qtc_sbom_supplier "")

  set_property(GLOBAL PROPERTY _qtc_sbom_default_copyrights "")
  set_property(GLOBAL PROPERTY _qtc_sbom_default_license_expression "")
  set_property(GLOBAL PROPERTY _qtc_sbom_default_version "")

  set_property(GLOBAL PROPERTY _qtc_sbom_no_default_copyrights "")
  set_property(GLOBAL PROPERTY _qtc_sbom_no_default_license_expression "")
  set_property(GLOBAL PROPERTY _qtc_sbom_no_default_version "")

  set_property(GLOBAL PROPERTY _qtc_sbom_targets_expecting_finalization "")

  qtc_reset_exported_sbom_paths_for_current_project()
endfunction()

# Gets the name of the dir where SBOMs should be installed.
function(qtc_sbom_get_sbom_install_dir out_var)
  if(QTC_SBOM_INSTALL_DIR)
    set(sbom_dir "${QTC_SBOM_INSTALL_DIR}")
  else()
    set(sbom_dir "sbom")
  endif()

  set(${out_var} "${sbom_dir}" PARENT_SCOPE)
endfunction()

# Gets the relative install path where Creator specific SBOMs should be installed.
function(qtc_sbom_get_sbom_install_path out_var)
  qtc_sbom_get_sbom_install_dir(sbom_dir)

  if(QTC_SBOM_INSTALL_PATH)
    set(out_path "${QTC_SBOM_INSTALL_PATH}/${sbom_dir}")
  elseif(IDE_DATA_PATH)
    set(out_path "${IDE_DATA_PATH}/${sbom_dir}")
  elseif(CMAKE_INSTALL_DATAROOTDIR)
    set(out_path "${CMAKE_INSTALL_DATAROOTDIR}/${sbom_dir}")
  else()
    set(out_path "share/${sbom_dir}")
  endif()

  set(${out_var} "${out_path}" PARENT_SCOPE)
endfunction()

# This function is meant to be called inside a Config.cmake file, to tell Qt about extra sbom paths
# that should be searched when finding dependency sbom documents.
# The paths currently need to be handled by the Creator-specific qtc_sbom_begin_project explicitly,
# but in the future Qt should do that automatically.
function(qtc_add_exported_sbom_paths)
  if(NOT QT_GENERATE_SBOM)
    return()
  endif()

  set(opt_args "")
  set(single_args "")
  set(multi_args
    PREFIXES
  )

  cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
  qtc_validate_all_args_are_parsed(arg)

  set_property(GLOBAL APPEND PROPERTY _qt_internal_exported_sbom_prefixes ${arg_PREFIXES})
endfunction()

# Internal function to make Qt aware of extra sbom dirs that have documents.
function(qtc_add_exported_sbom_paths_for_current_project)
  if(NOT QT_GENERATE_SBOM)
    return()
  endif()

  # Backup current set of sbom dirs.
  get_cmake_property(build_sbom_dirs _qt_internal_sbom_dirs)
  if(NOT build_sbom_dirs)
    set(build_sbom_dirs "")
  endif()
  set_property(GLOBAL PROPERTY _qt_internal_sbom_dirs_backup "${build_sbom_dirs}")

  # Append the exported sbom paths to the sbom dirs.
  get_cmake_property(sbom_prefixes _qt_internal_exported_sbom_prefixes)
  if(sbom_prefixes)
    set_property(GLOBAL APPEND PROPERTY _qt_internal_sbom_dirs ${sbom_prefixes})
  endif()
endfunction()

# Internal function to reset the extra sbom dirs above.
function(qtc_reset_exported_sbom_paths_for_current_project)
  if(NOT QT_GENERATE_SBOM)
    return()
  endif()

  # Restore backup of sbom dirs.
  get_cmake_property(build_sbom_dirs _qt_internal_sbom_dirs_backup)
  if(NOT build_sbom_dirs)
    set(build_sbom_dirs "")
  endif()
  set_property(GLOBAL PROPERTY _qt_internal_sbom_dirs "${build_sbom_dirs}")
endfunction()

# Get the spdx id for the Qt Commercial license.
function(qtc_sbom_get_default_commercial_license out_var)
  _qt_internal_sbom_get_spdx_license_expression("QT_COMMERCIAL" license)
  set(${out_var} "${license}" PARENT_SCOPE)
endfunction()

# Get the spdx id for the Qt Commercial + GPL3 exception license.
function(qtc_sbom_get_default_open_source_license out_var)
  _qt_internal_sbom_get_spdx_license_expression("QT_COMMERCIAL_OR_GPL3_WITH_EXCEPTION" license)
  set(${out_var} "${license}" PARENT_SCOPE)
endfunction()

# Get the Qt Company default copyright text.
function(qtc_sbom_get_default_copyright out_var)
  if(IDE_COPYRIGHT)
    set(value "${IDE_COPYRIGHT}")
  else()
    set(value "Copyright (C) The Qt Company Ltd. and other contributors.")
  endif()
  set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

# Gets the default Qt Creator HTTPS scheme repo URL: aka https://code.qt.io
function(qtc_sbom_get_qtc_vcs_url_base_default)
  set(opt_args "")
  set(single_args
    OUT_VAR
  )
  set(multi_args "")
  cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
  qtc_validate_all_args_are_parsed(arg)

  if(NOT arg_OUT_VAR)
    message(FATAL_ERROR "OUT_VAR must be set")
  endif()

  set(vcs_url "https://code.qt.io/qt-creator/qt-creator.git")
  set(${arg_OUT_VAR} "${vcs_url}" PARENT_SCOPE)
endfunction()

# Gets the VCS url that was configured for the current sbom project.
# This might different from the Qt Creator code.qt.io, when building standalone
# Creator plugins.
function(qtc_sbom_get_qtc_vcs_url target)
  set(opt_args "")
  set(single_args
    VERSION
    OUT_VAR
  )
  set(multi_args "")
  cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
  qtc_validate_all_args_are_parsed(arg)

  if(NOT arg_OUT_VAR)
    message(FATAL_ERROR "OUT_VAR must be set")
  endif()

  set(version_part "")
  if(arg_VERSION)
    set(version_part "@${arg_VERSION}")
  endif()

  get_cmake_property(vcs_url _qtc_sbom_vcs_base_url)
  string(APPEND vcs_url "${version_part}")
  set(${arg_OUT_VAR} "${vcs_url}" PARENT_SCOPE)
endfunction()

# Gets the default Qt Creator git scheme repo URL: aka git://code.qt.io
function(qtc_sbom_get_repo_source_download_location_default)
  set(opt_args "")
  set(single_args
    OUT_VAR
  )
  set(multi_args "")
  cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
  qtc_validate_all_args_are_parsed(arg)

  if(NOT arg_OUT_VAR)
    message(FATAL_ERROR "OUT_VAR must be set")
  endif()

  set(download_location "git://code.qt.io/qt-creator/qt-creator.git")

  set(${arg_OUT_VAR} "${download_location}" PARENT_SCOPE)
endfunction()

# Gets the download location that was configured for the current sbom project.
function(qtc_sbom_get_repo_source_download_location)
  set(opt_args
    ADD_CURRENT_SOURCE_DIR_SUB_PATH
  )
  set(single_args
    TARGET
    OUT_VAR
  )
  set(multi_args "")
  cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
  qtc_validate_all_args_are_parsed(arg)

  get_cmake_property(download_location _qtc_sbom_download_location_base)
  if(NOT download_location)
    set(${arg_OUT_VAR} "" PARENT_SCOPE)
    return()
  endif()

  _qt_internal_sbom_get_git_version_vars()
  if(QT_SBOM_GIT_HASH)
    string(APPEND download_location "@${QT_SBOM_GIT_HASH}")
  endif()

  if(arg_ADD_CURRENT_SOURCE_DIR_SUB_PATH AND arg_TARGET)
    # Add the subdirectory path where the target was created as a custom qualifier.
    _qt_internal_sbom_get_qt_entity_repo_source_dir("${arg_TARGET}" OUT_VAR sub_path)
    if(sub_path)
      string(APPEND download_location "#${sub_path}")
    endif()
  endif()

  set(${arg_OUT_VAR} "${download_location}" PARENT_SCOPE)
endfunction()

# Gets the args to create a specific type of PURL. Either GENERIC, GIT, or GITHUB.
# GIT is not officialy in the PURL spec, but some tools still scan it.
# Exits early if the purl namespace or name have not been previously set by the project begin
# command.
function(qtc_sbom_get_purl target)
  set(opt_args "")
  set(single_args
    PURL_TYPE
    OUT_VAR
  )
  set(multi_args "")
  cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
  qtc_validate_all_args_are_parsed(arg)

  if(NOT arg_PURL_TYPE)
    message(FATAL_ERROR "PURL_TYPE must be set")
  endif()

  if(NOT arg_OUT_VAR)
    message(FATAL_ERROR "OUT_VAR must be set")
  endif()

  set(purl_args "")

  if(arg_PURL_TYPE STREQUAL "GENERIC")
    list(APPEND purl_args
      PURL_ID "GENERIC"
      PURL_TYPE "generic"
    )
  elseif(arg_PURL_TYPE STREQUAL "GIT")
    list(APPEND purl_args
      PURL_ID "GIT"
      PURL_TYPE "git"
    )
  elseif(arg_PURL_TYPE STREQUAL "GITHUB")
    list(APPEND purl_args
      PURL_ID "GITHUB"
      PURL_TYPE "github"
    )
  else()
    message(FATAL_ERROR "Invalid PURL_TYPE: ${arg_PURL_TYPE}")
  endif()

  get_cmake_property(purl_name _qtc_sbom_purl_name)
  get_cmake_property(purl_namespace _qtc_sbom_purl_namespace)
  if(NOT purl_name OR NOT purl_namespace)
    set(${arg_OUT_VAR} "" PARENT_SCOPE)
    return()
  endif()

  _qt_internal_sbom_get_git_version_vars()

  set(entity_vcs_url_version_option "")
  # Can be empty.
  if(QT_SBOM_GIT_HASH_SHORT)
    set(entity_vcs_url_version_option VERSION "${QT_SBOM_GIT_HASH_SHORT}")
  endif()

  qtc_sbom_get_qtc_vcs_url(${target}
    ${entity_vcs_url_version_option}
    OUT_VAR vcs_url)
  list(APPEND purl_args PURL_QUALIFIERS "vcs_url=${vcs_url}")

  # Add the subdirectory path where the target was created as a custom qualifier.
  _qt_internal_sbom_get_qt_entity_repo_source_dir("${target}" OUT_VAR sub_path)
  if(sub_path)
    list(APPEND purl_args PURL_SUBPATH "${sub_path}")
  endif()

  # Add the target name as a custom qualifer.
  list(APPEND purl_args PURL_QUALIFIERS "library_name=${target}")

  # Can be empty.
  if(QT_SBOM_GIT_HASH_SHORT)
    list(APPEND purl_args PURL_VERSION "${QT_SBOM_GIT_HASH_SHORT}")
  endif()

  list(APPEND purl_args
    PURL_NAME "${purl_name}-${target}"
    PURL_NAMESPACE "${purl_namespace}"
  )

  set(${arg_OUT_VAR} "${purl_args}" PARENT_SCOPE)
endfunction()

# Internal function that is called by add_qtc_executable and friends to forward sbom related args.
function(qtc_extend_qtc_entity_sbom target)
  if(NOT QT_GENERATE_SBOM)
    return()
  endif()

  # Get the sbom entity type if we have relevant values in the passed args.
  qtc_sbom_add_sbom_entity_type_to_args(args_to_forward ${ARGN})

  # Get Creator project specific sbom args.
  qtc_sbom_get_default_sbom_args("${target}" sbom_extra_args ${args_to_forward})

  # Pass args to the Qt function.
  _qt_internal_extend_sbom(${target} ${args_to_forward} ${sbom_extra_args})
endfunction()

# Gets the default sbom args for Qt Creator entity types.
# Things like copyrights, licenses, download location, VCS url, CPE, PURL, etc.
function(qtc_sbom_get_default_sbom_args target out_var)
  _qt_internal_get_sbom_specific_options(opt_args single_args multi_args)
  list(APPEND opt_args NO_INSTALL)
  list(APPEND single_args TYPE)

  cmake_parse_arguments(PARSE_ARGV 2 arg "${opt_args}" "${single_args}" "${multi_args}")
  qtc_validate_all_args_are_parsed(arg)

  set(sbom_args "")

  # Skip adding Qt creator specific versions, licenses or copyrights to 3rd party entity types,
  # aka bundled / vendored libraries like lua.
  set(third_party_types
    QT_THIRD_PARTY_MODULE
    QT_THIRD_PARTY_SOURCES
    THIRD_PARTY_LIBRARY
    THIRD_PARTY_LIBRARY_WITH_FILES
    THIRD_PARTY_SOURCES
  )
  if(arg_TYPE IN_LIST third_party_types)
    set(is_third_party_type TRUE)
  else()
    set(is_third_party_type FALSE)
  endif()

  set(package_version "")
  get_cmake_property(project_no_default_version _qtc_sbom_no_default_version)
  if(NOT project_no_default_version AND NOT is_third_party_type)
    get_cmake_property(project_default_version _qtc_sbom_default_version)
    if(project_default_version)
      set(package_version "${project_default_version}")
    elseif(IDE_VERSION)
      set(package_version "${IDE_VERSION}")
    endif()
  endif()

  if(package_version)
    list(APPEND sbom_args PACKAGE_VERSION "${package_version}")
  endif()

  set(license_expression "")
  get_cmake_property(project_no_default_license_expression _qtc_sbom_no_default_license_expression)
  if(NOT project_no_default_license_expression AND NOT is_third_party_type)
    get_cmake_property(project_default_license_expression _qtc_sbom_default_license_expression)
    if(project_default_license_expression)
      set(license_expression "${project_default_license_expression}")
    else()
      qtc_sbom_get_default_open_source_license(license_expression)
    endif()
  endif()

  if(license_expression)
    list(APPEND sbom_args LICENSE_EXPRESSION "${license_expression}")
  endif()

  set(copyright "")
  get_cmake_property(project_no_default_copyrights _qtc_sbom_no_default_copyrights)
  if(NOT project_no_default_copyrights AND NOT is_third_party_type)
    get_cmake_property(project_default_copyrights _qtc_sbom_default_copyrights)
    if(project_default_copyrights)
      set(copyright "${project_default_copyrights}")
    else()
      qtc_sbom_get_default_copyright(copyright)
    endif()
  endif()

  if(copyright)
    list(APPEND sbom_args COPYRIGHTS "${copyright}")
  endif()

  get_cmake_property(supplier _qtc_sbom_supplier)
  if(supplier)
    list(APPEND sbom_args SUPPLIER "${supplier}")
  endif()

  get_cmake_property(cpe _qtc_sbom_cpe)
  if(cpe)
    list(APPEND sbom_args CPE "${cpe}")
  endif()

  qtc_sbom_get_repo_source_download_location(
    ADD_CURRENT_SOURCE_DIR_SUB_PATH
    TARGET "${target}"
    OUT_VAR download_location
  )
  if(download_location)
    list(APPEND sbom_args DOWNLOAD_LOCATION "${download_location}")
  endif()

  set(purl_entries "")

  qtc_sbom_get_purl(${target} PURL_TYPE GITHUB OUT_VAR purl_github_args)
  if(purl_github_args)
    list(APPEND purl_entries PURL_ENTRY ${purl_github_args})
  endif()

  qtc_sbom_get_purl(${target} PURL_TYPE GENERIC OUT_VAR purl_generic_args)
  if(purl_generic_args)
    list(APPEND purl_entries PURL_ENTRY ${purl_generic_args})
  endif()

  if(purl_entries)
    list(APPEND sbom_args PURLS ${purl_entries})
  endif()

  set(${out_var} "${sbom_args}" PARENT_SCOPE)
endfunction()

# Constructs a CPE string based on the passed vendor, product and version.
function(qtc_sbom_compute_cpe out_var)
  set(opt_args "")
  set(single_args
    VENDOR
    PRODUCT
    VERSION
  )
  set(multi_args "")

  cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")

  if(NOT QT_GENERATE_SBOM)
    set(${out_var} "" PARENT_SCOPE)
    return()
  endif()

  set(cpe_args "")

  if(arg_VENDOR)
    list(APPEND cpe_args VENDOR "${arg_VENDOR}")
  else()
    message(FATAL_ERROR "VENDOR must be set")
  endif()

  if(arg_PRODUCT)
    list(APPEND cpe_args PRODUCT "${arg_PRODUCT}")
  else()
    message(FATAL_ERROR "PRODUCT must be set")
  endif()

  if(arg_VERSION)
    list(APPEND cpe_args VERSION "${arg_VERSION}")
  endif()

  _qt_internal_sbom_compute_security_cpe(cpe ${cpe_args})
  set(${out_var} "${cpe}" PARENT_SCOPE)
endfunction()

# Transforms a DEFAULT_SBOM_ENTITY_TYPE or SBOM_ENTITY_TYPE to a TYPE, and adds that to the out_var
# forwarded args. Needed because the Qt SBOM API only knows about the ill-named TYPE option,
# rather than SBOM_ENTITY_TYPE, so we need to do this translation.
function(qtc_sbom_add_sbom_entity_type_to_args out_var)
  set(opt_args "")
  set(single_args
    DEFAULT_SBOM_ENTITY_TYPE
    SBOM_ENTITY_TYPE
  )
  set(multi_args "")

  cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")

  if(arg_SBOM_ENTITY_TYPE)
    set(sbom_entity_type "${arg_SBOM_ENTITY_TYPE}")
  elseif(arg_DEFAULT_SBOM_ENTITY_TYPE)
    set(sbom_entity_type "${arg_DEFAULT_SBOM_ENTITY_TYPE}")
  else()
    set(sbom_entity_type "")
  endif()

  # Remove the SBOM_ENTITY_TYPE and DEFAULT_SBOM_ENTITY_TYPE single-arg options, because
  # _qt_internal_extend_sbom doesn't know about them.
  qtc_remove_single_args(filtered_args
    ARGS ${ARGN}
    ARGS_TO_REMOVE SBOM_ENTITY_TYPE DEFAULT_SBOM_ENTITY_TYPE
  )

  # Forward the sbom entity type if we have one.
  if(sbom_entity_type)
    list(APPEND filtered_args TYPE "${sbom_entity_type}")
  endif()

  set(${out_var} "${filtered_args}" PARENT_SCOPE)
endfunction()

# Forwarding wrappers that should be used in CMakeLists.txt files.
# No-ops in case when sbom generation is not enabled, or when an older
# Qt version that doesn't have sbom support is used.

# Function to create a custom SBOM target that is not backed by a library / executable.
# Useful for things like translations, resources, etc.
function(qtc_add_sbom)
  if(NOT QT_GENERATE_SBOM)
    return()
  endif()

  _qt_internal_add_sbom(${ARGN})
endfunction()

# Function to add files to an SBOM-enabled target.
function(qtc_sbom_add_files)
  if(NOT QT_GENERATE_SBOM)
    return()
  endif()

  _qt_internal_sbom_add_files(${ARGN})
endfunction()

# Function to add additional SBOM related options to an SBOM-enabled target.
function(qtc_extend_sbom)
  if(NOT QT_GENERATE_SBOM)
    return()
  endif()

  qtc_extend_qtc_entity_sbom(${ARGN})
endfunction()
