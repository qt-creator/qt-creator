add_qtc_test(tst_qtprofiler_sampletrace
  DEPENDS CommonTraceFormat Utils
  DEFINES QMLPROFILER_STATIC_LIBRARY
  INCLUDES
    ../../../../src/plugins/profiler
    ../../../../src/plugins  # for qmlprofiler/profilertr.h (Tr context)
  SOURCES
    tst_sampletrace.cpp
    ../../../../src/plugins/profiler/sampletrace.cpp
)
