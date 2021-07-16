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
  perfconfigwidget.cpp perfconfigwidget.h
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

if(${Qt5_VERSION} VERSION_LESS "6.2.0")
  add_qtc_plugin(PerfProfiler
    DEPENDS Tracing Qt5::QuickWidgets
    PLUGIN_DEPENDS Core Debugger ProjectExplorer QtSupport
    SOURCES
      ${TEST_SOURCES}
      ${PERFPROFILER_CPP_SOURCES}
      perfprofiler.qrc
  )
else() # < Qt 6.2
  add_qtc_plugin(PerfProfiler
    DEPENDS Tracing Qt5::QuickWidgets
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
    NO_CREATE_PLUGIN_TARGET
    QML_FILES
      PerfProfilerFlameGraphView.qml
    SOURCES
      ${PERFPROFILER_CPP_SOURCES}
  )
endif() # < Qt 6.2
