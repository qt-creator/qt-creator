add_qtc_test(tst_qtprofiler_callstacklanemodel
  DEPENDS CommonTraceFormat Tracing Utils Qt::Gui
  DEFINES PROFILER_STATIC_LIBRARY
  INCLUDES "${PluginsDir}/profiler" "${PluginsDir}"
  SOURCES
    tst_callstacklanemodel.cpp
    ../../../../src/plugins/profiler/callstacklanemodel.cpp
    ../../../../src/plugins/profiler/sampletrace.cpp
)
