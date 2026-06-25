add_qtc_test(tst_qtprofiler_calltreemodel
  DEPENDS CommonTraceFormat Utils
  DEFINES QMLPROFILER_STATIC_LIBRARY
  INCLUDES ../../../../src/plugins/profiler ../../../../src/plugins
  SOURCES
    tst_calltreemodel.cpp
    ../../../../src/plugins/profiler/calltreemodel.cpp
    ../../../../src/plugins/profiler/sampletrace.cpp
)
