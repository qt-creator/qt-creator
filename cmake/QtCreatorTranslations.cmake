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
          list(FILTER _include_dirs EXCLUDE REGEX "\\$<TARGET_PROPERTY")
          list(FILTER _include_dirs EXCLUDE REGEX "\\$<INSTALL_INTERFACE")
          list(TRANSFORM _include_dirs REPLACE "\\$<BUILD_INTERFACE:([^>]+)>" "\\1")
          list(APPEND _includes ${_include_dirs})
        endif()

        if(_interface_include_dirs)
          list(APPEND _interface_includes ${_include_dirs})
        endif()

        set(_target_sources "")
        if(_source_files)
          # exclude various funny source files, and anything generated
          # like *metatypes.json.gen, moc_*.cpp, qrc_*.cpp, */qmlcache/*.cpp,
          # *qmltyperegistrations.cpp
          string(REGEX REPLACE "(\\^|\\$|\\.|\\[|\\]|\\*|\\+|\\?|\\(|\\)|\\|)" "\\\\\\1" binary_dir_regex "${PROJECT_BINARY_DIR}")
          set(_exclude_patterns
            .*[.]json[.]in
            .*[.]svg
            .*[.]pro
            .*[.]pri
            .*[.]css
            "${binary_dir_regex}/.*"
          )
          list(JOIN _exclude_patterns "|" _exclude_pattern)
          list(FILTER _source_files EXCLUDE REGEX "${_exclude_pattern}")
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

function(_create_lupdate_response_file response_file)
  set(no_value_options "")
  set(single_value_options "")
  set(multi_value_options SOURCES INCLUDES)
  cmake_parse_arguments(PARSE_ARGV 1 arg
    "${no_value_options}" "${single_value_options}" "${multi_value_options}"
  )
  if(arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
  endif()

  set(sources "${arg_SOURCES}")
  list(SORT sources)

  set(includes "${arg_INCLUDES}")

  list(REMOVE_DUPLICATES sources)
  list(REMOVE_DUPLICATES includes)

  list(REMOVE_ITEM sources "")
  list(REMOVE_ITEM includes "")

  list(TRANSFORM includes PREPEND "-I")

  string(REPLACE ";" "\n" sources_str "${sources}")
  string(REPLACE ";" "\n" includes_str "${includes}")

  file(WRITE "${response_file}" "${sources_str}\n${includes_str}")
endfunction()

function(_create_ts_custom_target name)
  cmake_parse_arguments(_arg "" "FILE_PREFIX;LUPDATE_RESPONSE_FILE;TS_TARGET_PREFIX"
    "DEPENDS;LANGUAGES" ${ARGN}
  )
  if (_arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Invalid parameters to _create_ts_custom_target: ${_arg_UNPARSED_ARGUMENTS}.")
  endif()

  if (NOT _arg_TS_TARGET_PREFIX)
    set(_arg_TS_TARGET_PREFIX "ts_")
  endif()

  set(languages "${name}")
  if(DEFINED _arg_LANGUAGES)
    set(languages ${_arg_LANGUAGES})
  endif()

  set(ts_files "")
  foreach(language IN LISTS languages)
    list(APPEND ts_files "${CMAKE_CURRENT_SOURCE_DIR}/${_arg_FILE_PREFIX}_${language}.ts")
  endforeach()

  set(common_comment "Generate .ts file")
  list(LENGTH languages languages_length)
  if(languages_length GREATER 1)
    string(APPEND common_comment "s")
  endif()
  string(APPEND common_comment " (${name})")

  set(response_file ${_arg_LUPDATE_RESPONSE_FILE})
  add_custom_target("${_arg_TS_TARGET_PREFIX}${name}"
    COMMAND Qt::lupdate -locations relative -no-ui-lines "@${response_file}" -ts ${ts_files}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "${common_comment}, with obsolete translations and files and line numbers"
    DEPENDS ${_arg_DEPENDS}
    VERBATIM)

  add_custom_target("${_arg_TS_TARGET_PREFIX}${name}_no_locations"
    COMMAND Qt::lupdate -locations none -no-ui-lines "@${response_file}" -ts ${ts_files}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "${common_comment}, with obsolete translations, without files and line numbers"
    DEPENDS ${_arg_DEPENDS}
    VERBATIM)

  # Uses lupdate + convert instead of just lupdate with '-locations none -no-obsolete'
  # to keep the same sorting as the non-'cleaned' target and therefore keep the diff small
  set(lconvert_commands "")
  foreach(ts_file IN LISTS ts_files)
    list(APPEND lconvert_commands
      COMMAND Qt::lconvert -locations none -no-ui-lines -no-obsolete ${ts_file} -o ${ts_file}
    )
  endforeach()

  add_custom_target("${_arg_TS_TARGET_PREFIX}${name}_cleaned"
    COMMAND Qt::lupdate -locations relative -no-ui-lines "@${response_file}" -ts ${ts_files}
    ${lconvert_commands}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "${common_comment}, remove obsolete and vanished translations, and do not add files and line number"
    DEPENDS ${_arg_DEPENDS}
    VERBATIM)
endfunction()

function(add_translation_targets file_prefix)
  if(NOT TARGET Qt::lrelease OR NOT TARGET Qt::lupdate OR NOT TARGET Qt::lconvert)
    # No Qt translation tools were found: Skip this directory
    message(WARNING "No Qt translation tools found, skipping translation targets. Add find_package(Qt6 COMPONENTS LinguistTools) to CMake to enable.")
    return()
  endif()

  set(opt_args "")
  set(single_args
    OUTPUT_DIRECTORY
    INSTALL_DESTINATION
    TS_TARGET_PREFIX
    QM_TARGET_PREFIX
    ALL_QM_TARGET
  )
  set(multi_args
    TS_LANGUAGES
    QM_LANGUAGES
    TARGETS
    SOURCES
    INCLUDES
  )

  cmake_parse_arguments(_arg "${opt_args}" "${single_args}" "${multi_args}" ${ARGN})
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

  set(lupdate_response_file "${CMAKE_CURRENT_BINARY_DIR}/lupdate-args.lst")
  _create_lupdate_response_file(${lupdate_response_file}
    SOURCES ${_to_process_sources} ${_arg_SOURCES}
    INCLUDES ${_to_process_includes} ${_arg_INCLUDES}
  )

  set(ts_languages untranslated ${_arg_TS_LANGUAGES})
  foreach(language IN LISTS ts_languages)
    _create_ts_custom_target(${language}
      FILE_PREFIX "${file_prefix}" TS_TARGET_PREFIX "${_arg_TS_TARGET_PREFIX}"
      LUPDATE_RESPONSE_FILE "${lupdate_response_file}"
      DEPENDS ${_arg_SOURCES}
    )
  endforeach()

  # Create ts_all* targets.
  _create_ts_custom_target(all
    LANGUAGES ${ts_languages}
    FILE_PREFIX "${file_prefix}"
    TS_TARGET_PREFIX "${_arg_TS_TARGET_PREFIX}"
    LUPDATE_RESPONSE_FILE "${lupdate_response_file}"
    DEPENDS ${_arg_SOURCES}
  )

  if (NOT TARGET "${_arg_ALL_QM_TARGET}")
    add_custom_target("${_arg_ALL_QM_TARGET}" ALL COMMENT "Generate .qm-files")
  endif()

  file(MAKE_DIRECTORY ${_arg_OUTPUT_DIRECTORY})

  foreach(l IN LISTS _arg_QM_LANGUAGES)
    set(_ts_file "${CMAKE_CURRENT_SOURCE_DIR}/${file_prefix}_${l}.ts")
    set(_qm_file "${_arg_OUTPUT_DIRECTORY}/${file_prefix}_${l}.qm")

    add_custom_command(OUTPUT "${_qm_file}"
      COMMAND Qt::lrelease "${_ts_file}" -qm "${_qm_file}"
      MAIN_DEPENDENCY "${_ts_file}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMENT "Generate .qm file"
      VERBATIM)
    add_custom_target("${_arg_QM_TARGET_PREFIX}${l}" DEPENDS "${_qm_file}")
    install(FILES "${_qm_file}" DESTINATION ${_arg_INSTALL_DESTINATION})

    add_dependencies("${_arg_ALL_QM_TARGET}" "${_arg_QM_TARGET_PREFIX}${l}")
  endforeach()
endfunction()
