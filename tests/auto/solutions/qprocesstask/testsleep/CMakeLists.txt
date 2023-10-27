add_qtc_executable(testsleep
  SOURCES main.cpp
  SKIP_INSTALL
  INTERNAL_ONLY
)

set_target_properties(testsleep PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
