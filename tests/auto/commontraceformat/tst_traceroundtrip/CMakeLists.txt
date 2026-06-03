add_qtc_test(tst_ctf2_traceroundtrip
  DEPENDS CommonTraceFormat
  DEFINES TESTDATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data"
  SOURCES tst_traceroundtrip.cpp
)
