if(WITH_TESTS)
  set(TEST_SOURCES
    tests/perfprofilertracefile_test.cpp tests/perfprofilertracefile_test.h
    tests/perfresourcecounter_test.cpp tests/perfresourcecounter_test.h
    tests/tests.qrc
  )
else()
  set(TEST_SOURCES "")
endif()

set(PERFPROFILER_CPP_SOURCES
  perfconfigeventsmodel.cpp perfconfigeventsmodel.h
  perfdatareader.cpp perfdatareader.h
  perfevent.h
  perfeventtype.h
  perfloaddialog.cpp perfloaddialog.h
  perfprofiler.qrc
  perfprofilerconstants.h
  perfprofilerflamegraphmodel.cpp perfprofilerflamegraphmodel.h
  perfprofilerflamegraphview.cpp perfprofilerflamegraphview.h
  perfprofilerplugin.cpp perfprofilerplugin.h
  perfprofilerruncontrol.cpp perfprofilerruncontrol.h
  perfprofilerstatisticsmodel.cpp perfprofilerstatisticsmodel.h
  perfprofilerstatisticsview.cpp perfprofilerstatisticsview.h
  perfprofilertool.cpp perfprofilertool.h
  perfprofilertracefile.cpp perfprofilertracefile.h
  perfprofilertracemanager.cpp perfprofilertracemanager.h
  perfprofilertraceview.cpp perfprofilertraceview.h
  perfprofiler_global.h
  perfprofilertr.h
  perfresourcecounter.cpp perfresourcecounter.h
  perfrunconfigurationaspect.cpp perfrunconfigurationaspect.h
  perfsettings.cpp perfsettings.h
  perftimelinemodel.cpp perftimelinemodel.h
  perftimelinemodelmanager.cpp perftimelinemodelmanager.h
  perftimelineresourcesrenderpass.cpp perftimelineresourcesrenderpass.h
  perftracepointdialog.cpp perftracepointdialog.h
)

add_qtc_plugin(PerfProfiler
  CONDITION TARGET Tracing
  DEPENDS Tracing Qt::QuickWidgets
  PLUGIN_DEPENDS Core Debugger ProjectExplorer QtSupport
  SOURCES
    ${TEST_SOURCES}
)

if (NOT TARGET PerfProfiler)
  return()
endif()

qt_add_resources(PerfProfiler perfprofiler
  PREFIX "/perfprofiler"
  tracepoints.sh
)

qt_add_qml_module(PerfProfiler
  URI "QtCreator.PerfProfiler"
  VERSION "1.0"
  NO_PLUGIN
  RESOURCE_PREFIX "/qt/qml"
  QML_FILES
    PerfProfilerFlameGraphView.qml
  SOURCES
    ${PERFPROFILER_CPP_SOURCES}
)
