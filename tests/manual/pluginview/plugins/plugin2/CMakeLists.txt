add_library(tst_manual_plugin2 SHARED
  plugin2.cpp plugin2.h
)

set_target_properties(tst_manual_plugin2 PROPERTIES
  OUTPUT_NAME "plugin2"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.."
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.."
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.."
)

target_link_libraries(tst_manual_plugin2 PRIVATE ExtensionSystem)
