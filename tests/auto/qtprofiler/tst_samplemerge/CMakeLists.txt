add_qtc_test(tst_qtprofiler_samplemerge
  DEPENDS CommonTraceFormat Utils
  DEFINES PROFILER_STATIC_LIBRARY
  INCLUDES ../../../../src/plugins/profiler ../../../../src/plugins
  SOURCES
    tst_samplemerge.cpp
    ../../../../src/plugins/profiler/samplemerge.cpp
    ../../../../src/plugins/profiler/sampletrace.cpp
)
