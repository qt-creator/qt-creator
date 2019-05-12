add_qtc_test(tst_json
  DEPENDS Utils
  INCLUDES "${CMAKE_SOURCE_DIR}/src/shared/json"
  SOURCES
    tst_json.cpp
    "${CMAKE_SOURCE_DIR}/src/shared/json/json.cpp"
)
