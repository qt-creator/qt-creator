add_qtc_test(tst_json
  DEPENDS Utils
  INCLUDES "${PROJECT_SOURCE_DIR}/src/shared/json"
  SOURCES
    tst_json.cpp
    "${PROJECT_SOURCE_DIR}/src/shared/json/json.cpp"
)
