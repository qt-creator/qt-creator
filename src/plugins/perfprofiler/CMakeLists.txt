add_qtc_plugin(PerfProfiler
  DEPENDS Tracing
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport
  SOURCES
    perfconfigeventsmodel.cpp perfconfigeventsmodel.h
    perfdatareader.cpp perfdatareader.h
    perfevent.h
    perfeventtype.h
    perfloaddialog.cpp perfloaddialog.h
    perfprofiler.qrc
    perfprofilerconstants.h
    perfprofilerflamegraphmodel.cpp perfprofilerflamegraphmodel.h
    perfprofilerflamegraphview.cpp perfprofilerflamegraphview.h
    perfprofilerplugin.cpp
    perfprofilerruncontrol.cpp perfprofilerruncontrol.h
    perfprofilerstatisticsmodel.cpp perfprofilerstatisticsmodel.h
    perfprofilerstatisticsview.cpp perfprofilerstatisticsview.h
    perfprofilertool.cpp perfprofilertool.h
    perfprofilertracefile.cpp perfprofilertracefile.h
    perfprofilertracemanager.cpp perfprofilertracemanager.h
    perfprofiler_global.h
    perfprofilertr.h
    perfresourcecounter.h
    perfrunconfigurationaspect.cpp perfrunconfigurationaspect.h
    perfsettings.cpp perfsettings.h
    perftimelinemodel.cpp perftimelinemodel.h
    perftimelinemodelmanager.cpp perftimelinemodelmanager.h
    perftracepointdialog.cpp perftracepointdialog.h
)

extend_qtc_plugin(PerfProfiler
  CONDITION WITH_TESTS
  SOURCES
    tests/perfprofilertracefile_test.cpp tests/perfprofilertracefile_test.h
    tests/perfresourcecounter_test.cpp tests/perfresourcecounter_test.h
    tests/perfnativemixed_test.cpp tests/perfnativemixed_test.h
    tests/tests.qrc
)

qt_add_resources(PerfProfiler perfprofiler
  PREFIX "/perfprofiler"
  tracepoints.sh
)
