set(template_directories cplusplus debugger glsl modeleditor qml qmldesigner
  qmlicons qml-type-descriptions schemes scripts snippets styles templates themes welcomescreen)

add_custom_target(copy_share_to_builddir ALL
  COMMENT Copy files into build directory
  VERBATIM
)

foreach(dir IN ITEMS ${template_directories})
  add_custom_command(TARGET copy_share_to_builddir POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${dir}"
      "${PROJECT_BINARY_DIR}/${IDE_DATA_PATH}/${dir}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT Copy files into build directory
    VERBATIM
  )
endforeach()

install(DIRECTORY ${template_directories} DESTINATION "${IDE_DATA_PATH}")
install(
  FILES indexer_preincludes/qglobal.h indexer_preincludes/windows.h
  DESTINATION "${IDE_DATA_PATH}/indexer_preincludes"
)

add_subdirectory(translations)
