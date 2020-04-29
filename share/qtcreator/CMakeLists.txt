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

# copy resource directories during build
qtc_copy_to_builddir(copy_share_to_builddir
  DIRECTORIES ${resource_directories}
  DESTINATION "${IDE_DATA_PATH}"
  CREATE_SUBDIRS
)

# create install rule for resource directories
install(DIRECTORY ${resource_directories} DESTINATION "${IDE_DATA_PATH}")

add_subdirectory(translations)
