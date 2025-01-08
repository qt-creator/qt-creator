# copy resource directories during build
qtc_copy_to_builddir(copy_changelog_to_builddir
  DIRECTORIES changelog
  DESTINATION "${IDE_DATA_PATH}"
  CREATE_SUBDIRS
)

install(
  DIRECTORY changelog
  DESTINATION ${IDE_DATA_PATH}
)

file(GLOB_RECURSE changelog_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "changelog/*")

qtc_add_sbom(Changelog
    SBOM_ENTITY_TYPE RESOURCES
)

qtc_sbom_add_files(Changelog
    FILES "${changelog_files}"
    FILE_TYPE "RESOURCES"
    INSTALL_PATH "${IDE_DATA_PATH}/changelog"
)

