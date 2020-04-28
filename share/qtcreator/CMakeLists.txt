set(resource_directories
  android
  cplusplus
  debugger
  glsl
  indexer_preincludes
  modeleditor
  qml
  qmldesigner
  qmlicons
  qml-type-descriptions
  schemes
  snippets
  styles
  templates
  themes
)

if (APPLE)
  set(resource_directories ${resource_directories} scripts)
endif()

add_custom_target(copy_share_to_builddir ALL
  COMMENT Copy files into build directory
  VERBATIM
)

# copy resource directories during build
foreach(dir IN ITEMS ${resource_directories})
  add_custom_command(TARGET copy_share_to_builddir POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${dir}"
      "${PROJECT_BINARY_DIR}/${IDE_DATA_PATH}/${dir}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT Copy resource directories into build directory
    VERBATIM
  )
endforeach()

# create install rule for resource directories
install(DIRECTORY ${resource_directories} DESTINATION "${IDE_DATA_PATH}")

add_subdirectory(translations)
