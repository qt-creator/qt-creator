set(resource_directories
  android
  cplusplus
  glsl
  indexer_preincludes
  jsonschemas
  lua-plugins
  lua-lupdate
  modeleditor
  qmldesigner
  qmlicons
  qml-type-descriptions
  schemes
  snippets
  styles
  templates
  themes
)

set(resource_files
  debugger/.pylintrc
  debugger/boosttypes.py
  debugger/cdbbridge.py
  debugger/cdbext.pyi
  debugger/creatortypes.py
  debugger/dumper.py
  debugger/gdbbridge.py
  debugger/gdbtracepoint.py
  debugger/LICENSE.GPL3-EXCEPT
  debugger/lldbbridge.py
  debugger/misctypes.py
  debugger/opencvtypes.py
  debugger/pdbbridge.py
  debugger/personaltypes.py
  debugger/qttypes.py
  debugger/README.txt
  debugger/setup.cfg
  debugger/android_stdtypes.py
  debugger/libcpp_stdtypes.py
  debugger/stdtypes.py
  debugger/utils.py
  debugger/loadorder_gdbbridge.txt
  debugger/loadorder_lldbbridge.txt
  debugger/visualize.py
)

# copy resource directories during build
qtc_copy_to_builddir(copy_share_to_builddir
  DIRECTORIES ${resource_directories}
  DESTINATION "${IDE_DATA_PATH}"
  CREATE_SUBDIRS
)

# copy resource files during build
qtc_copy_to_builddir(copy_share_files_to_builddir
  FILES ${resource_files}
  DESTINATION ${IDE_DATA_PATH}
)

# create install rule for resource directories
install(
  DIRECTORY ${resource_directories}
  DESTINATION "${IDE_DATA_PATH}"
  USE_SOURCE_PERMISSIONS
)

qtc_add_sbom(CreatorShareResources
  SBOM_ENTITY_TYPE RESOURCES
  USE_ATTRIBUTION_FILES
  CREATE_SBOM_FOR_EACH_ATTRIBUTION
  ATTRIBUTION_FILE_PATHS
    "${QtCreator_SOURCE_DIR}/qt_attributions.json"
  ATTRIBUTION_IDS
    pyprof2calltree
)

# create install rule for resource files
foreach(file ${resource_files})
  get_filename_component(path "${file}" DIRECTORY)
  set(dest "${IDE_DATA_PATH}/${path}")
  install(
    FILES "${file}"
    DESTINATION "${dest}"
  )
  qtc_sbom_add_files(CreatorShareResources
    FILES "${file}"
    FILE_TYPE "RESOURCES"
    INSTALL_PATH "${dest}"
  )
endforeach()

qtc_sbom_add_files(CreatorShareResources
  DIRECTORIES "${resource_directories}"
  FILE_TYPE "RESOURCES"
  INSTALL_PATH "${IDE_DATA_PATH}"
)

add_subdirectory(translations)
