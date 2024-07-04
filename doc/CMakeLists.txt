# Generate documentation

option(BUILD_DEVELOPER_DOCS "Include developer documentation" OFF)
add_feature_info("Include developer documentation" BUILD_DEVELOPER_DOCS "")

function(_find_all_includes _ret_includes _ret_framework_paths)
  set(_all_includes "${PROJECT_SOURCE_DIR}/src/plugins;${PROJECT_SOURCE_DIR}/src/libs")
  foreach(_target ${__QTC_PLUGINS} ${__QTC_LIBRARIES})
    if (NOT TARGET ${_target})
      continue()
    endif()
    get_target_property(_includes ${_target} INCLUDE_DIRECTORIES)
    foreach(_include ${_includes})
      string(FIND "${_include}" "/src/plugins/" _in_plugins)
      string(FIND "${_include}" "/src/libs/" _in_libs)
      string(FIND "${_include}" "${CMAKE_BINARY_DIR}" _in_build)
      if(_in_plugins LESS 0 AND _in_libs LESS 0 AND _in_build LESS 0)
        list(APPEND _all_includes ${_include})
      endif()
    endforeach()
  endforeach()
  list(APPEND _all_includes ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
  list(REMOVE_DUPLICATES _all_includes)
  set("${_ret_includes}" "${_all_includes}" PARENT_SCOPE)

  # framework path
  if (APPLE)
    get_target_property(_qt_type Qt::Core TYPE)
    if ("${_qt_type}" STREQUAL "INTERFACE_LIBRARY")
        # version-less target Qt::Core is an interface library that links to QtX::Core
        get_target_property(_qt_core Qt::Core INTERFACE_LINK_LIBRARIES)
    else()
        set(_qt_core "Qt::Core")
    endif()
    get_target_property(_qt_target ${_qt_core} LOCATION) # <fw_path>/QtCore.framework/QtCore
    get_filename_component(_qt_loc "${_qt_target}" DIRECTORY)
    set("${_ret_framework_paths}" "${_qt_loc}/.." PARENT_SCOPE)
  endif()
endfunction()

function(_add_doc _doc_file _dev_doc_file)
  set(GENERATED_ATTRIBUTIONS_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated_attributions)
  add_qtc_documentation(${_doc_file}
    ENVIRONMENT_EXPORTS GENERATED_ATTRIBUTIONS_DIR
  )
  add_qtc_doc_attribution(doc_attributions
    "${CMAKE_CURRENT_SOURCE_DIR}/../qt_attributions.json"
    "${GENERATED_ATTRIBUTIONS_DIR}/creator-attributions.qdoc"
    ${_doc_file}
  )
  set(litehtml_attributions "${CMAKE_CURRENT_SOURCE_DIR}/../src/libs/qlitehtml/src/3rdparty/qt_attribution.json")
  if(EXISTS "${litehtml_attributions}")
    add_qtc_doc_attribution(doc_attributions_litehtml
      "${litehtml_attributions}"
      "${GENERATED_ATTRIBUTIONS_DIR}/creator-attributions-litehtml.qdoc"
      ${_doc_file}
    )
  endif()
  if (BUILD_DEVELOPER_DOCS)
    _find_all_includes(_all_includes _framework_paths)
    add_qtc_documentation(${_dev_doc_file}
      INCLUDE_DIRECTORIES ${_all_includes}
      FRAMEWORK_PATHS ${_framework_paths}
    )
  endif()
endfunction()

if (WITH_DOCS)
  _add_doc(${IDE_DOC_FILE} "qtcreatordev/qtcreator-dev.qdocconf")
endif()
if(WITH_ONLINE_DOCS)
  _add_doc(${IDE_DOC_FILE_ONLINE} "qtcreatordev/qtcreator-dev-online.qdocconf")
endif()

if (BUILD_DOCS_BY_DEFAULT)
set(EXCLUDE_DOCS_FROM_ALL "")
else()
set(EXCLUDE_DOCS_FROM_ALL "EXCLUDE_FROM_ALL")
endif()

install(DIRECTORY config
  DESTINATION ${IDE_HEADER_INSTALL_PATH}/doc
  COMPONENT Devel
  ${EXCLUDE_DOCS_FROM_ALL}
)
