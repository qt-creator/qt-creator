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
    get_target_property(_qt_target Qt5::Core LOCATION) # <fw_path>/QtCore.framework/QtCore
    get_filename_component(_qt_loc "${_qt_target}" DIRECTORY)
    set("${_ret_framework_paths}" "${_qt_loc}/.." PARENT_SCOPE)
  endif()
endfunction()

if (WITH_DOCS)
  add_qtc_documentation("qtcreator/qtcreator.qdocconf")
  if (BUILD_DEVELOPER_DOCS)
    _find_all_includes(_all_includes _framework_paths)
    add_qtc_documentation("qtcreatordev/qtcreator-dev.qdocconf"
      INCLUDE_DIRECTORIES ${_all_includes}
      FRAMEWORK_PATHS ${_framework_paths}
    )
  endif()
endif()
if(WITH_ONLINE_DOCS)
  add_qtc_documentation("qtcreator/qtcreator-online.qdocconf")
  if (BUILD_DEVELOPER_DOCS)
    _find_all_includes(_all_includes _framework_paths)
    add_qtc_documentation("qtcreatordev/qtcreator-dev-online.qdocconf"
      INCLUDE_DIRECTORIES ${_all_includes}
      FRAMEWORK_PATHS ${_framework_paths}
    )
  endif()
endif()

install(DIRECTORY config
  DESTINATION doc
  COMPONENT Devel
  EXCLUDE_FROM_ALL
)
