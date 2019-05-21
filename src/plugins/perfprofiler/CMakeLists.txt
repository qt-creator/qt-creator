add_qtc_plugin(PerfProfiler
  DEPENDS Tracing Qt5::QuickWidgets
  PLUGIN_DEPENDS Core Debugger ProjectExplorer QtSupport
  SOURCES
    perfconfigeventsmodel.cpp perfconfigeventsmodel.h
    perfconfigwidget.cpp perfconfigwidget.h
    perfconfigwidget.ui
    perfdatareader.cpp perfdatareader.h
    perfevent.h
    perfeventtype.h
    perfloaddialog.cpp perfloaddialog.h perfloaddialog.ui
    perfoptionspage.cpp perfoptionspage.h
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
    perfresourcecounter.cpp perfresourcecounter.h
    perfrunconfigurationaspect.cpp perfrunconfigurationaspect.h
    perfsettings.cpp perfsettings.h
    perftimelinemodel.cpp perftimelinemodel.h
    perftimelinemodelmanager.cpp perftimelinemodelmanager.h
    perftimelineresourcesrenderpass.cpp perftimelineresourcesrenderpass.h
    perftracepointdialog.cpp perftracepointdialog.h perftracepointdialog.ui
)

extend_qtc_plugin(PerfProfiler
  CONDITION WITH_TESTS
  SOURCES
    tests/perfprofilertracefile_test.cpp tests/perfprofilertracefile_test.h
    tests/perfresourcecounter_test.cpp tests/perfresourcecounter_test.h
    tests/tests.qrc
)
