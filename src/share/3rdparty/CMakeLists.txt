install(
  DIRECTORY fonts
  DESTINATION "${IDE_DATA_PATH}"
)

qtc_copy_to_builddir(copy_fonts_to_builddir
	DIRECTORIES fonts
	DESTINATION "${IDE_DATA_PATH}/fonts"
)
