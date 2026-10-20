add_qtc_test(tst_qtprofiler_sampletrace
  DEPENDS CommonTraceFormat Utils
  DEFINES PROFILER_STATIC_LIBRARY
  INCLUDES
    "${PluginsDir}/profiler"
    "${PluginsDir}"  # for qmlprofiler/profilertr.h (Tr context)
  SOURCES
    tst_sampletrace.cpp
    ../../../../src/plugins/profiler/sampletrace.cpp
)
