# Options:
option(WITH_DOCS "Build documentation" OFF)
add_feature_info("Build documentation" WITH_DOCS "")

option(WITH_ONLINE_DOCS "Build online documentation" OFF)
add_feature_info("Build online documentation" WITH_ONLINE_DOCS "")

# Get information on directories from qmake
# as this is not yet exported by cmake.
# Used for QT_INSTALL_DOCS
function(qt5_query_qmake)
  if (NOT TARGET Qt5::qmake)
    message(FATAL_ERROR "Qmake was not found. Add find_package(Qt5 COMPONENTS Core) to CMake to enable.")
  endif()
  # dummy check for if we already queried qmake
  if (QT_INSTALL_BINS)
    return()
  endif()

  get_target_property(_qmake_binary Qt5::qmake IMPORTED_LOCATION)
  execute_process(COMMAND "${_qmake_binary}" "-query"
                  TIMEOUT 10
                  RESULT_VARIABLE _qmake_result
                  OUTPUT_VARIABLE _qmake_stdout
                  OUTPUT_STRIP_TRAILING_WHITESPACE)

  if (NOT "${_qmake_result}" STREQUAL "0")
    message(FATAL_ERROR "Qmake did not execute successfully: ${_qmake_result}.")
  endif()

  # split into lines:
  string(REPLACE "\n" ";" _lines "${_qmake_stdout}")

  foreach(_line ${_lines})
    # split line into key/value pairs
    string(REPLACE ":" ";" _parts "${_line}")
    list(GET _parts 0 _key)
    list(REMOVE_AT _parts 0)
    string(REPLACE ";" ":" _value "${_parts}")

    set("${_key}" "${_value}" CACHE PATH "qmake import of ${_key}" FORCE)
  endforeach()
endfunction()

# Find programs:
function(_doc_find_program result_var)
  if (NOT TARGET Qt5::qmake)
    message(FATAL_ERROR "QDoc is only available in Qt5 projects")
  endif()

  get_target_property(_qmake_binary Qt5::qmake IMPORTED_LOCATION)
  get_filename_component(_qmake_dir "${_qmake_binary}" DIRECTORY)
  find_program("_prg_${result_var}" ${ARGN} HINTS "${_qmake_dir}")
  if ("_prg_${result_var}" STREQUAL "_prg_${result_var}-NOTFOUND")
    set("_prg_${result_var}" "${result_var}-NOTFOUND")
    message(WARNING "Could not find binary for ${result_var}")
  endif()

  set(${result_var} "${_prg_${result_var}}" PARENT_SCOPE)
endfunction()

function(_setup_doc_targets)
  # Set up important targets:
  if (NOT TARGET html_docs)
    add_custom_target(html_docs COMMENT "Build HTML documentation")
  endif()
  if (NOT TARGET qch_docs)
    add_custom_target(qch_docs COMMENT "Build QCH documentation")
  endif()
  if (NOT TARGET docs)
    add_custom_target(docs COMMENT "Build documentation")
    add_dependencies(docs html_docs qch_docs)
  endif()
endfunction()

function(_setup_qdoc_targets _qdocconf_file _retval)
  cmake_parse_arguments(_arg "" "HTML_DIR;INSTALL_DIR;POSTFIX"
    "INDEXES;INCLUDE_DIRECTORIES;FRAMEWORK_PATHS;ENVIRONMENT_EXPORTS" ${ARGN})

  foreach(_index ${_arg_INDEXES})
    list(APPEND _qdoc_index_args "-indexdir;${_index}")
  endforeach()

  set(_env "")
  foreach(_export ${_arg_ENVIRONMENT_EXPORTS})
    if (NOT DEFINED "${_export}")
      message(FATAL_ERROR "${_export} is not known when trying to export it to qdoc.")
    endif()
    list(APPEND _env "${_export}=${${_export}}")
  endforeach()

  set(_full_qdoc_command "${_qdoc}")
  if (_env)
    set(_full_qdoc_command "${CMAKE_COMMAND}" "-E" "env" ${_env} "${_qdoc}")
  endif()

  if (_arg_HTML_DIR STREQUAL "")
    set(_arg_HTML_DIR "${CMAKE_CURRENT_BINARY_DIR}/doc")
  endif()

  get_filename_component(_target "${_qdocconf_file}" NAME_WE)

  set(_html_outputdir "${_arg_HTML_DIR}/${_target}${_arg_POSTFIX}")
  file(MAKE_DIRECTORY "${_html_outputdir}")

  set(_qdoc_include_args "")
  if (_arg_INCLUDE_DIRECTORIES OR _arg_FRAMEWORK_PATHS)
    # pass include directories to qdoc via hidden @ option, since we need to generate a file
    # to be able to resolve the generators inside the include paths
    set(_qdoc_includes "${CMAKE_CURRENT_BINARY_DIR}/cmake/qdoc_${_target}.inc")
    set(_qdoc_include_args "@${_qdoc_includes}")
    set(_includes "")
    if (_arg_INCLUDE_DIRECTORIES)
      set(_includes "-I$<JOIN:${_arg_INCLUDE_DIRECTORIES},\n-I>\n")
    endif()
    set(_frameworks "")
    if (_arg_FRAMEWORK_PATHS)
      set(_frameworks "-F$<JOIN:${_arg_FRAMEWORK_PATHS},\n-F>\n")
    endif()
    file(GENERATE
      OUTPUT "${_qdoc_includes}"
      CONTENT "${_includes}${_frameworks}"
    )
  endif()

  set(_html_target "html_docs_${_target}")
  add_custom_target("${_html_target}"
      ${_full_qdoc_command} -outputdir "${_html_outputdir}" "${_qdocconf_file}"
      ${_qdoc_index_args} ${_qdoc_include_args}
    COMMENT "Build HTML documentation from ${_qdocconf_file}"
    DEPENDS "${_qdocconf_file}"
    SOURCES "${_qdocconf_file}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
  )
  add_dependencies(html_docs "${_html_target}")

  # Install HTML files as a special component
  install(DIRECTORY "${_html_outputdir}" DESTINATION "${_arg_INSTALL_DIR}"
    COMPONENT html_docs EXCLUDE_FROM_ALL)

  set("${_retval}" "${_html_outputdir}" PARENT_SCOPE)
endfunction()

function(_setup_qhelpgenerator_targets _qdocconf_file _html_outputdir)
  cmake_parse_arguments(_arg "" "QCH_DIR;INSTALL_DIR" "" ${ARGN})
  if (_arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "qdoc_build_qdocconf_file has unknown arguments: ${_arg_UNPARSED_ARGUMENTS}.")
  endif()

  if (NOT _arg_QCH_DIR)
    set(_arg_QCH_DIR "${CMAKE_CURRENT_BINARY_DIR}/doc")
  endif()

  if (NOT TARGET Qt5::qhelpgenerator)
    message(WARNING "qhelpgenerator missing: No QCH documentation targets were generated. Add find_package(Qt5 COMPONENTS Help) to CMake to enable.")
    return()
  endif()

  get_filename_component(_target "${_qdocconf_file}" NAME_WE)

  set(_qch_outputdir "${_arg_QCH_DIR}")
  file(MAKE_DIRECTORY "${_qch_outputdir}")

  set(_qch_target "qch_docs_${_target}")
  set(_html_target "html_docs_${_target}")
  add_custom_target("${_qch_target}"
    Qt5::qhelpgenerator "${_html_outputdir}/${_target}.qhp" -o "${_qch_outputdir}/${_target}.qch"
    COMMENT "Build QCH documentation from ${_qdocconf_file}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
  )
  add_dependencies("${_qch_target}" "${_html_target}")
  add_dependencies(qch_docs "${_qch_target}")

  install(FILES "${_qch_outputdir}/${_target}.qch" DESTINATION "${_arg_INSTALL_DIR}"
    COMPONENT qch_docs EXCLUDE_FROM_ALL)
endfunction()

# Helper functions:
function(qdoc_build_qdocconf_file _qdocconf_file)
  _setup_doc_targets()

  _doc_find_program(_qdoc NAMES qdoc qdoc-qt5)
  if (_qdoc STREQUAL "_qdoc-NOTFOUND")
     message(WARNING "No qdoc binary found: No documentation targets were generated")
     return()
  endif()

  cmake_parse_arguments(_arg "QCH" "HTML_DIR;QCH_DIR;INSTALL_DIR;POSTFIX"
    "INDEXES;INCLUDE_DIRECTORIES;FRAMEWORK_PATHS;ENVIRONMENT_EXPORTS" ${ARGN})
  if (_arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "qdoc_build_qdocconf_file has unknown arguments: ${_arg_UNPARSED_ARGUMENTS}.")
  endif()

  if (NOT _arg_INSTALL_DIR)
    message(FATAL_ERROR "No INSTALL_DIR set when calling qdoc_build_qdocconf_file")
  endif()

  _setup_qdoc_targets("${_qdocconf_file}" _html_outputdir
    HTML_DIR "${_arg_HTML_DIR}" INSTALL_DIR "${_arg_INSTALL_DIR}"
    INDEXES ${_arg_INDEXES} ENVIRONMENT_EXPORTS ${_arg_ENVIRONMENT_EXPORTS}
    POSTFIX "${_arg_POSTFIX}"
    INCLUDE_DIRECTORIES ${_arg_INCLUDE_DIRECTORIES}
    FRAMEWORK_PATHS ${_arg_FRAMEWORK_PATHS}
  )

  if (_arg_QCH)
    _setup_qhelpgenerator_targets("${_qdocconf_file}" "${_html_outputdir}"
      QCH_DIR "${_arg_QCH_DIR}" INSTALL_DIR "${_arg_INSTALL_DIR}")
  endif()
endfunction()

set(QtCreatorDocumentation_LIST_DIR ${CMAKE_CURRENT_LIST_DIR})

function(qtc_docs_dir varName)
  if (QtCreator_SOURCE_DIR)
    # Qt Creator build or super-repo
    set(${varName} "${QtCreator_SOURCE_DIR}/doc" PARENT_SCOPE)
  elseif(QtCreatorDocumentation_LIST_DIR MATCHES /lib/cmake/QtCreator$)
    # Dev package
    set(${varName} "${QtCreatorDocumentation_LIST_DIR}/../../../doc" PARENT_SCOPE)
  else()
    message(FATAL_ERROR "Could not find qtc_docs_dir")
  endif()
endfunction()

function(qtc_index_dir varName)
  if (QtCreator_BINARY_DIR)
    # Qt Creator build or super-repo
    set(${varName} "${QtCreator_BINARY_DIR}/doc/html" PARENT_SCOPE)
  elseif(QtCreatorDocumentation_LIST_DIR MATCHES /lib/cmake/QtCreator$)
    # Dev package
    set(${varName} "${QtCreatorDocumentation_LIST_DIR}/../../../${IDE_DOC_PATH}" PARENT_SCOPE)
  else()
    message(FATAL_ERROR "Could not find qtc_index_dir")
  endif()
endfunction()

function(add_qtc_documentation qdocconf_file)
  cmake_parse_arguments(_arg "" ""
    "INCLUDE_DIRECTORIES;FRAMEWORK_PATHS" ${ARGN})
  if (_arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "add_qtc_documentation has unknown arguments: ${_arg_UNPARSED_ARGUMENTS}.")
  endif()

  ### Skip docs setup if that is not needed!
  if (NOT WITH_ONLINE_DOCS AND NOT WITH_DOCS)
    return()
  endif()


  qt5_query_qmake()
  qtc_output_binary_dir(_output_binary_dir)
  qtc_docs_dir(QTC_DOCS_DIR)
  qtc_index_dir(QTC_INDEX_DIR)

  set(_qch_params)
  if (WITH_DOCS)
    set(_qch_params QCH QCH_DIR "${_output_binary_dir}/${IDE_DOC_PATH}")
  endif()
  set(_qdoc_params HTML_DIR "${_output_binary_dir}/doc/html")
  list(APPEND _qdoc_params INDEXES "${QT_INSTALL_DOCS}" "${QTC_INDEX_DIR}")
  list(APPEND _qdoc_params INSTALL_DIR "${IDE_DOC_PATH}")

  # Set up environment for qdoc:
  set(QTC_VERSION "${IDE_VERSION_DISPLAY}")
  set(QTCREATOR_COPYRIGHT_YEAR "${IDE_COPYRIGHT_YEAR}")
  string(REPLACE "." "" QTC_VERSION_TAG "${IDE_VERSION}")
  set(QDOC_INDEX_DIR "${QT_INSTALL_DOCS}")
  if (QT_INSTALL_DOCS_src)
    set(QT_INSTALL_DOCS "${QT_INSTALL_DOCS_src}")
  endif()
  list(APPEND _qdoc_params ENVIRONMENT_EXPORTS
    IDE_ID IDE_CASED_ID IDE_DISPLAY_NAME
    QTC_DOCS_DIR QTC_VERSION QTC_VERSION_TAG
    QTCREATOR_COPYRIGHT_YEAR
    QT_INSTALL_DOCS QDOC_INDEX_DIR
  )

  qdoc_build_qdocconf_file(${qdocconf_file} ${_qch_params} ${_qdoc_params}
    INCLUDE_DIRECTORIES ${_arg_INCLUDE_DIRECTORIES}
    FRAMEWORK_PATHS ${_arg_FRAMEWORK_PATHS}
  )
endfunction()
