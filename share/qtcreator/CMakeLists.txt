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
  debugger/loadorder.txt
  debugger/visualize.py

  debugger-with-python2/qttypes.py
  debugger-with-python2/personaltypes.py
  debugger-with-python2/android_stdtypes.py
  debugger-with-python2/README.txt
  debugger-with-python2/gdbbridge.py
  debugger-with-python2/cdbbridge.py
  debugger-with-python2/lldbbridge.py
  debugger-with-python2/misctypes.py
  debugger-with-python2/opencvtypes.py
  debugger-with-python2/libcpp_stdtypes.py
  debugger-with-python2/creatortypes.py
  debugger-with-python2/stdtypes.py
  debugger-with-python2/gdbtracepoint.py
  debugger-with-python2/utils.py
  debugger-with-python2/boosttypes.py
  debugger-with-python2/dumper.py
  debugger-with-python2/pdbbridge.py
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
  TYPE RESOURCES
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
