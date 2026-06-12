add_qtc_test(tst_qmltraceviewer_cpuusagemodel
  DEPENDS CommonTraceFormat Tracing Utils Qt::Gui
  DEFINES QMLPROFILER_STATIC_LIBRARY
  INCLUDES ../../../../src/plugins/profiler ../../../../src/plugins
  SOURCES
    tst_cpuusagemodel.cpp
    ../../../../src/plugins/profiler/cpuusagemodel.cpp
    ../../../../src/plugins/profiler/sampletrace.cpp
)
