# Generate documentation

# Options:
option(WITH_DOCS "Build documentation" OFF)
add_feature_info("Build documentation" WITH_DOCS "")

option(WITH_ONLINE_DOCS "Build online documentation" OFF)
add_feature_info("Build online documentation" WITH_ONLINE_DOCS "")

option(BUILD_DEVELOPER_DOCS "Include developer documentation" OFF)
add_feature_info("Include developer documentation" BUILD_DEVELOPER_DOCS "")


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
  if (NOT TARGET install_docs)
    add_custom_target(install_docs COMMENT "Install documentation")
    add_dependencies(install_docs docs)
  endif()
  if (NOT TARGET clean_docs)
    add_custom_target(clean_docs COMMENT "Clean documentation files from build directory")
  endif()
endfunction()

function(_setup_qdoc_targets _qdocconf_file _retval)
  cmake_parse_arguments(_arg "" "HTML_DIR;INSTALL_DIR;POSTFIX"
    "INDEXES;ENVIRONMENT_EXPORTS" ${ARGN})

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

  set(_html_target "html_docs_${_target}")
  add_custom_target("${_html_target}"
    ${_full_qdoc_command} "-outputdir" "${_html_outputdir}" "${_qdocconf_file}" ${_qdoc_index_args}
    COMMENT "Build HTML documentation from ${_qdocconf_file}"
    DEPENDS "${_qdocconf_file}"
    SOURCES "${_qdocconf_file}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
  )
  add_dependencies(html_docs "${_html_target}")

  # Install HTML files as a special component
  install(DIRECTORY "${_html_outputdir}" DESTINATION "${_arg_INSTALL_DIR}"
    COMPONENT ${_html_target} EXCLUDE_FROM_ALL)

  add_custom_target("install_${_html_target}"
    "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=${_html_target}
      -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
    COMMENT "Install HTML documentation from ${_qdocconf_file}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
  )
  add_dependencies(install_docs "install_${_html_target}")

  add_custom_target("clean_${_html_target}"
    "${CMAKE_COMMAND}" -E remove_directory "${_html_outputdir}"
    COMMENT "Clean HTML documentation from ${_qdocconf_file}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
  )
  add_dependencies(clean_docs "clean_${_html_target}")

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
    message(WARNING "qhelpgenerator missing: No QCH documentation targets were generated")
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
    COMPONENT ${_qch_target} EXCLUDE_FROM_ALL)

  add_custom_target("install_${_qch_target}"
    "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=${_qch_target}
      -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
  )
  add_dependencies(install_docs "install_${_qch_target}")

  add_custom_target("clean_${_qch_target}"
    "${CMAKE_COMMAND}" -E remove -f "${_qch_outputdir}/${_target}.qch"
    COMMENT "Clean QCH documentation generated from ${_qdocconf_file}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
  )
  add_dependencies(clean_docs "clean_${_qch_target}")
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
    "INDEXES;ENVIRONMENT_EXPORTS" ${ARGN})
  if (_arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "qdoc_build_qdocconf_file has unknown arguments: ${_arg_UNPARSED_ARGUMENTS}.")
  endif()

  if (NOT _arg_INSTALL_DIR)
    message(FATAL_ERROR "No INSTALL_DIR set when calling qdoc_build_qdocconf_file")
  endif()

  _setup_qdoc_targets("${_qdocconf_file}" _html_outputdir
    HTML_DIR "${_arg_HTML_DIR}" INSTALL_DIR "${_arg_INSTALL_DIR}"
    INDEXES ${_arg_INDEXES} ENVIRONMENT_EXPORTS ${_arg_ENVIRONMENT_EXPORTS}
    POSTFIX "${_arg_POSTFIX}")

  if (_arg_QCH)
    _setup_qhelpgenerator_targets("${_qdocconf_file}" "${_html_outputdir}"
      QCH_DIR "${_arg_QCH_DIR}" INSTALL_DIR "${_arg_INSTALL_DIR}")
  endif()
endfunction()

### Skip docs setup if that is not needed!
if (WITH_ONLINE_DOCS OR WITH_DOCS)
  if (WITH_DOCS)
    set(_qch_params "QCH;QCH_DIR;${PROJECT_BINARY_DIR}/doc")
  endif()

  set(_qdoc_params HTML_DIR "${PROJECT_BINARY_DIR}/doc")
  list(APPEND _qdoc_params INDEXES "${QT_INSTALL_DOCS}" "${PROJECT_BINARY_DIR}/doc")
  list(APPEND _qdoc_params INSTALL_DIR "${IDE_DOC_PATH}")

  # Set up environment for qdoc:
  set(QTC_VERSION "${IDE_VERSION_DISPLAY}")
  set(QTCREATOR_COPYRIGHT_YEAR "${IDE_COPYRIGHT_YEAR}")
  set(IDE_SOURCE_TREE "${PROJECT_SOURCE_DIR}")
  string(REPLACE "." "" QTC_VERSION_TAG "${IDE_VERSION}")
  set(QTC_DOCS_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  set(QDOC_INDEX_DIR "${QT_INSTALL_DOCS}")
  if (QT_INSTALL_DOCS_src)
    set(QT_INSTALL_DOCS "${QT_INSTALL_DOCS_src}")
  endif()

  list(APPEND _qdoc_params ENVIRONMENT_EXPORTS
    IDE_ID IDE_CASED_ID IDE_DISPLAY_NAME

    IDE_SOURCE_TREE

    QTC_DOCS_DIR QTC_VERSION QTC_VERSION_TAG

    QTCREATOR_COPYRIGHT_YEAR

    QT_INSTALL_DOCS QDOC_INDEX_DIR)

  if (WITH_DOCS)
    qdoc_build_qdocconf_file("qtcreator.qdocconf" ${_qch_params} ${_qdoc_params})
    if (BUILD_DEVELOPER_DOCS)
      qdoc_build_qdocconf_file("api/qtcreator-dev.qdocconf" ${_qch_params} ${_qdoc_params})
    endif()
  endif()
  if(WITH_ONLINE_DOCS)
    qdoc_build_qdocconf_file("qtcreator-online.qdocconf" ${_qdoc_params})
    if (BUILD_DEVELOPER_DOCS)
      qdoc_build_qdocconf_file("api/qtcreator-dev-online.qdocconf" ${_qdoc_params})
    endif()
  endif()
endif()
