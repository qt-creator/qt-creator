add_qtc_test(tst_profilewriter
  DEPENDS Utils ProParser
  INCLUDES "${CMAKE_SOURCE_DIR}/src/shared/proparser"
  SOURCES tst_profilewriter.cpp
)
