add_library(tst_manual_plugin1 SHARED
  plugin1.cpp plugin1.h
)

set_target_properties(tst_manual_plugin1 PROPERTIES
  OUTPUT_NAME "plugin1"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.."
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.."
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.."
)

target_link_libraries(tst_manual_plugin1 PRIVATE ExtensionSystem
                                         PRIVATE tst_manual_plugin2
                                         PRIVATE tst_manual_plugin3)
