install(
  DIRECTORY fonts
  DESTINATION "${IDE_DATA_PATH}"
)

add_custom_target(copy_fonts_to_builddir ALL VERBATIM)
add_custom_command(TARGET copy_fonts_to_builddir POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_directory fonts
    "${PROJECT_BINARY_DIR}/${IDE_DATA_PATH}/fonts"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  COMMENT Copy files into build directory
  VERBATIM
)
