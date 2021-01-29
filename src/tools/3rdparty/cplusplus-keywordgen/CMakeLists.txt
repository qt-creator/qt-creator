add_qtc_executable(cplusplus-keywordgen
  DEPENDS CPlusPlus Utils
  SOURCES cplusplus-keywordgen.cpp
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

