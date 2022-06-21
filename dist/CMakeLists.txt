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
