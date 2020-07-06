# Defines function add_translation_targets

function(_extract_ts_data_from_targets outprefix)
  set(_sources "")
  set(_includes "")

  set(_targets "${ARGN}")
  list(REMOVE_DUPLICATES _targets)

  foreach(t IN ITEMS ${_targets})
    if (TARGET "${t}")
      get_target_property(_skip_translation "${t}" QT_SKIP_TRANSLATION)
      get_target_property(_source_dir "${t}" SOURCE_DIR)
      get_target_property(_interface_include_dirs "${t}" INTERFACE_INCLUDE_DIRECTORIES)
      get_target_property(_include_dirs "${t}" INCLUDE_DIRECTORIES)
      get_target_property(_source_files "${t}" SOURCES)
      get_target_property(_extra_translations "${t}" QT_EXTRA_TRANSLATIONS)

      if (NOT _skip_translation)
        if(_include_dirs)
          list(APPEND _includes ${_include_dirs})
        endif()

        if(_interface_include_dirs)
          list(APPEND _interface_includes ${_include_dirs})
        endif()

        set(_target_sources "")
        if(_source_files)
          list(APPEND _target_sources ${_source_files})
        endif()
        if(_extra_translations)
          list(APPEND _target_sources ${_extra_translations})
        endif()
        foreach(s IN ITEMS ${_target_sources})
          get_filename_component(_abs_source "${s}" ABSOLUTE BASE_DIR "${_source_dir}")
          list(APPEND _sources "${_abs_source}")
        endforeach()
      endif()
    endif()
  endforeach()

  set("${outprefix}_sources" "${_sources}" PARENT_SCOPE)
  set("${outprefix}_includes" "${_includes}" PARENT_SCOPE)
endfunction()

function(_create_ts_custom_target name)
  cmake_parse_arguments(_arg "" "FILE_PREFIX;TS_TARGET_PREFIX" "LANGUAGES;SOURCES;INCLUDES" ${ARGN})
  if (_arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Invalid parameters to _create_ts_custom_target: ${_arg_UNPARSED_ARGUMENTS}.")
  endif()

  if (NOT _arg_TS_TARGET_PREFIX)
    set(_arg_TS_TARGET_PREFIX "ts_")
  endif()

  set(ts_languages ${_arg_LANGUAGES})
  if (NOT ts_languages)
    set(ts_languages "${name}")
  endif()

  foreach(l IN ITEMS ${ts_languages})
    list(APPEND ts_files "${CMAKE_CURRENT_SOURCE_DIR}/${_arg_FILE_PREFIX}_${l}.ts")
  endforeach()

  set(_sources "${_arg_SOURCES}")
  list(SORT _sources)

  set(_includes "${_arg_INCLUDES}")

  list(REMOVE_DUPLICATES _sources)
  list(REMOVE_DUPLICATES _includes)

  list(REMOVE_ITEM _sources "")
  list(REMOVE_ITEM _includes "")

  set(_prepended_includes)
  foreach(include IN LISTS _includes)
    list(APPEND _prepended_includes "-I${include}")
  endforeach()
  set(_includes "${_prepended_includes}")

  string(REPLACE ";" "\n" _sources_str "${_sources}")
  string(REPLACE ";" "\n" _includes_str "${_includes}")

  set(ts_file_list "${CMAKE_CURRENT_BINARY_DIR}/ts_${name}.lst")
  file(WRITE "${ts_file_list}" "${_sources_str}\n${_includes_str}\n")

  add_custom_target("${_arg_TS_TARGET_PREFIX}${name}"
    COMMAND Qt5::lupdate -locations relative -no-ui-lines -no-sort "@${ts_file_list}" -ts ${ts_files}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Generate .ts files"
    DEPENDS ${_sources}
    VERBATIM)
endfunction()

function(add_translation_targets file_prefix)
  if (NOT TARGET Qt5::lrelease)
    # No Qt translation tools were found: Skip this directory
    message(WARNING "No Qt translation tools found, skipping translation targets. Add find_package(Qt5 COMPONENTS LinguistTools) to CMake to enable.")
    return()
  endif()

  cmake_parse_arguments(_arg ""
    "OUTPUT_DIRECTORY;INSTALL_DESTINATION;TS_TARGET_PREFIX;QM_TARGET_PREFIX;ALL_QM_TARGET"
    "LANGUAGES;TARGETS;SOURCES;INCLUDES" ${ARGN})
  if (_arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Invalid parameters to add_translation_targets: ${_arg_UNPARSED_ARGUMENTS}.")
  endif()

  if (NOT _arg_TS_TARGET_PREFIX)
    set(_arg_TS_TARGET_PREFIX "ts_")
  endif()

  if (NOT _arg_QM_TARGET_PREFIX)
    set(_arg_QM_TARGET_PREFIX "generate_qm_file_")
  endif()

  if (NOT _arg_ALL_QM_TARGET)
    set(_arg_ALL_QM_TARGET "generate_qm_files")
  endif()

  if (NOT _arg_OUTPUT_DIRECTORY)
    set(_arg_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
  endif()

  if (NOT _arg_INSTALL_DESTINATION)
    message(FATAL_ERROR "Please provide a INSTALL_DESTINATION to add_translation_targets")
  endif()

  _extract_ts_data_from_targets(_to_process "${_arg_TARGETS}")

  _create_ts_custom_target(untranslated
    FILE_PREFIX "${file_prefix}" TS_TARGET_PREFIX "${_arg_TS_TARGET_PREFIX}"
    SOURCES ${_to_process_sources} ${_arg_SOURCES} INCLUDES ${_to_process_includes} ${_arg_INCLUDES})

  if (NOT TARGET "${_arg_ALL_QM_TARGET}")
    add_custom_target("${_arg_ALL_QM_TARGET}" ALL COMMENT "Generate .qm-files")
  endif()

  file(MAKE_DIRECTORY ${_arg_OUTPUT_DIRECTORY})

  foreach(l IN ITEMS ${_arg_LANGUAGES})
    set(_ts_file "${CMAKE_CURRENT_SOURCE_DIR}/${file_prefix}_${l}.ts")
    set(_qm_file "${_arg_OUTPUT_DIRECTORY}/${file_prefix}_${l}.qm")

    _create_ts_custom_target("${l}" FILE_PREFIX "${file_prefix}" TS_TARGET_PREFIX "${_arg_TS_TARGET_PREFIX}"
      SOURCES ${_to_process_sources} ${_arg_SOURCES} INCLUDES ${_to_process_includes} ${_arg_INCLUDES})

    add_custom_command(OUTPUT "${_qm_file}"
      COMMAND Qt5::lrelease "${_ts_file}" -qm "${_qm_file}"
      MAIN_DEPENDENCY "${_ts_file}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMENT "Generate .qm file"
      VERBATIM)
    add_custom_target("${_arg_QM_TARGET_PREFIX}${l}" DEPENDS "${_qm_file}")
    install(FILES "${_qm_file}" DESTINATION ${_arg_INSTALL_DESTINATION})

    add_dependencies("${_arg_ALL_QM_TARGET}" "${_arg_QM_TARGET_PREFIX}${l}")
  endforeach()

  _create_ts_custom_target(all
    LANGUAGES ${_arg_LANGUAGES}
    TS_TARGET_PREFIX "${_arg_TS_TARGET_PREFIX}"
    FILE_PREFIX "${file_prefix}"
    SOURCES ${_to_process_sources} ${_arg_SOURCES}
    INCLUDES ${_to_process_includes} ${_arg_INCLUDES}
  )
endfunction()
