add_qtc_test(tst_acp
  EXCLUDE_FROM_PRECHECK
  DEPENDS AcpLib
  INCLUDES "${PROJECT_SOURCE_DIR}/src/libs"
  SOURCES tst_acp.cpp
)
