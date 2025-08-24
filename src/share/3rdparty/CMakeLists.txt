set(resource_directories
  fonts
  cmake-helper
)

# create install rule for resource directories
install(
  DIRECTORY ${resource_directories}
  DESTINATION "${IDE_DATA_PATH}"
  USE_SOURCE_PERMISSIONS
)

# copy resource directories during build
qtc_copy_to_builddir(copy_fonts_to_builddir
  DIRECTORIES ${resource_directories}
  DESTINATION "${IDE_DATA_PATH}"
  CREATE_SUBDIRS
)

file(GLOB_RECURSE cmake_helper_files cmake-helper/*)
qtc_add_sbom(CMakeHelperResources
  SBOM_ENTITY_TYPE RESOURCES
  LICENSE_EXPRESSION "MIT"
  USE_ATTRIBUTION_FILES
  ATTRIBUTION_FILE_PATHS
    "${QtCreator_SOURCE_DIR}/qt_attributions.json"
  ATTRIBUTION_IDS
    conan.cmake
)

qtc_sbom_add_files(CMakeHelperResources
  FILES "${cmake_helper_files}"
  FILE_TYPE "RESOURCES"
  INSTALL_PATH "${IDE_DATA_PATH}/cmake-helper"
)

file(GLOB_RECURSE fonts_files fonts/*)
qtc_add_sbom(FontsResources
  SBOM_ENTITY_TYPE RESOURCES
  LICENSE_EXPRESSION "OFL-1.1-RFN"
  USE_ATTRIBUTION_FILES
  ATTRIBUTION_FILE_PATHS
    "${QtCreator_SOURCE_DIR}/qt_attributions.json"
  ATTRIBUTION_IDS
    source-code-pro-font
)

qtc_sbom_add_files(FontsResources
  FILES "${fonts_files}"
  FILE_TYPE "RESOURCES"
  INSTALL_PATH "${IDE_DATA_PATH}/fonts"
)
