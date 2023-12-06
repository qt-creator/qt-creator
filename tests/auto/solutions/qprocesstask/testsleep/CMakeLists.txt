add_qtc_executable(testsleep
  SOURCES main.cpp
  SKIP_INSTALL
  INTERNAL_ONLY
)

extend_qtc_executable(testsleep
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
