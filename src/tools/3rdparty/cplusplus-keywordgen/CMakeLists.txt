add_qtc_executable(cplusplus-keywordgen
  SKIP_INSTALL
  DEPENDS CPlusPlus Utils
  SOURCES cplusplus-keywordgen.cpp
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  SBOM_ARGS
    USE_ATTRIBUTION_FILES
    CREATE_SBOM_FOR_EACH_ATTRIBUTION
    ATTRIBUTION_FILE_PATHS
      "${QtCreator_SOURCE_DIR}/qt_attributions.json"
    ATTRIBUTION_IDS
      cpp-keywordgen
)

