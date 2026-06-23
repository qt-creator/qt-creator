if(WITH_TESTS)
  set(TEST_SOURCES
    tests/debugmessagesmodel_test.cpp tests/debugmessagesmodel_test.h
    tests/fakedebugserver.cpp tests/fakedebugserver.h
    tests/flamegraphmodel_test.cpp tests/flamegraphmodel_test.h
    tests/flamegraphview_test.cpp tests/flamegraphview_test.h
    tests/inputeventsmodel_test.cpp tests/inputeventsmodel_test.h
    tests/localqmlprofilerrunner_test.cpp tests/localqmlprofilerrunner_test.h
    tests/memoryusagemodel_test.cpp tests/memoryusagemodel_test.h
    tests/pixmapcachemodel_test.cpp tests/pixmapcachemodel_test.h
    tests/qmlnote_test.cpp tests/qmlnote_test.h
    tests/qmlprofileranimationsmodel_test.cpp tests/qmlprofileranimationsmodel_test.h
    tests/qmlprofilerattachdialog_test.cpp tests/qmlprofilerattachdialog_test.h
    tests/qmlprofilerclientmanager_test.cpp tests/qmlprofilerclientmanager_test.h
    tests/qmlprofilerdetailsrewriter_test.cpp tests/qmlprofilerdetailsrewriter_test.h
    tests/qmlprofilertool_test.cpp tests/qmlprofilertool_test.h
    tests/qmlprofilertraceview_test.cpp tests/qmlprofilertraceview_test.h
    tests/tests.qrc
    tests/perfprofilertracefile_test.cpp tests/perfprofilertracefile_test.h
    tests/perfresourcecounter_test.cpp tests/perfresourcecounter_test.h
    tests/perfnativemixed_test.cpp tests/perfnativemixed_test.h
    tests/perfprofilertests.qrc
  )
else()
  set(TEST_SOURCES "")
endif()

set(QMLPROFILER_CPP_SOURCES
  ctfloader.cpp ctfloader.h
  callstacksampler.cpp callstacksampler.h
  calltreemodel.cpp calltreemodel.h
  calltreeview.cpp calltreeview.h
  cpuusagemodel.cpp cpuusagemodel.h
  macsampler.cpp macsampler.h
  sampler.h
  samplerviewmanager.cpp samplerviewmanager.h
  sampletrace.cpp sampletrace.h
  ctfplainviewmanager.cpp ctfplainviewmanager.h
  ctfstatisticsmodel.cpp ctfstatisticsmodel.h
  ctfstatisticsview.cpp ctfstatisticsview.h
  ctftimelinemodel.cpp ctftimelinemodel.h
  ctftracemanager.cpp ctftracemanager.h
  ctfvisualizerconstants.h
  ctfvisualizertool.cpp ctfvisualizertool.h
  debugmessagesmodel.cpp debugmessagesmodel.h
  flamegraphmodel.cpp flamegraphmodel.h
  flamegraphview.cpp flamegraphview.h
  inputeventsmodel.cpp inputeventsmodel.h
  memoryusagemodel.cpp memoryusagemodel.h
  pixmapcachemodel.cpp pixmapcachemodel.h
  qmlnote.cpp qmlnote.h
  profilertr.h
  profiler_global.h
  qmlprofileranimationsmodel.cpp qmlprofileranimationsmodel.h
  qmlprofilerattachdialog.cpp qmlprofilerattachdialog.h
  qmlprofilerclientmanager.cpp qmlprofilerclientmanager.h
  qmlprofilerconstants.h
  qmlprofilerdetailsrewriter.cpp qmlprofilerdetailsrewriter.h
  qmlprofilereventsview.h
  qmlprofilermodelmanager.cpp qmlprofilermodelmanager.h
  qmlprofilernotesmodel.cpp qmlprofilernotesmodel.h
  qmlprofilerplainviewmanager.cpp qmlprofilerplainviewmanager.h
  qmlprofilerplugin.cpp
  qmlprofilerrangemodel.cpp qmlprofilerrangemodel.h
  qmlprofilerrunconfigurationaspect.cpp qmlprofilerrunconfigurationaspect.h
  qmlprofilerruncontrol.cpp qmlprofilerruncontrol.h
  qmlprofilersettings.cpp qmlprofilersettings.h
  qmlprofilerstatemanager.cpp qmlprofilerstatemanager.h
  qmlprofilerstatewidget.cpp qmlprofilerstatewidget.h
  qmlprofilerstatisticsmodel.cpp qmlprofilerstatisticsmodel.h
  qmlprofilerstatisticsview.cpp qmlprofilerstatisticsview.h
  qmlprofilertimelinemodel.cpp qmlprofilertimelinemodel.h
  qmlprofilertool.cpp qmlprofilertool.h
  qmlprofilertracefile.cpp qmlprofilertracefile.h
  qmlprofilertraceview.cpp qmlprofilertraceview.h
  scenegraphtimelinemodel.cpp scenegraphtimelinemodel.h
  quick3dmodel.cpp quick3dmodel.h
  quick3dframeview.cpp quick3dframeview.h
  quick3dframemodel.cpp quick3dframemodel.h
)

# Formerly the separate PerfProfiler plugin, folded in so the perf and QML
# profilers share one tool/perspective/trace clock. Kept in the PerfProfiler
# namespace; see native-mixed-profiler-design.md.
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
  perfprofilerruncontrol.cpp perfprofilerruncontrol.h
  perfprofilerstatisticsmodel.cpp perfprofilerstatisticsmodel.h
  perfprofilerstatisticsview.cpp perfprofilerstatisticsview.h
  perfprofilertool.cpp perfprofilertool.h
  perfprofilertracefile.cpp perfprofilertracefile.h
  perfprofilertracemanager.cpp perfprofilertracemanager.h
  perfprofilertr.h
  perfresourcecounter.h
  perfrunconfigurationaspect.cpp perfrunconfigurationaspect.h
  perfsettings.cpp perfsettings.h
  perftimelinemodel.cpp perftimelinemodel.h
  perftimelinemodelmanager.cpp perftimelinemodelmanager.h
  perftracepointdialog.cpp perftracepointdialog.h
)

add_qtc_plugin(QmlProfiler
  DEPENDS QmlDebug QmlJS Tracing CommonTraceFormat QtTaskTree
  INCLUDES ${PROJECT_SOURCE_DIR}/src
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport TextEditor
  SOURCES
    ${TEST_SOURCES}
    ${QMLPROFILER_CPP_SOURCES}
    ${PERFPROFILER_CPP_SOURCES}
    ../../libs/3rdparty/json/json.hpp
  SBOM_ARGS
    USE_ATTRIBUTION_FILES
    CREATE_SBOM_FOR_EACH_ATTRIBUTION
    ATTRIBUTION_FILE_PATHS
      "${QtCreator_SOURCE_DIR}/qt_attributions.json"
    ATTRIBUTION_IDS
      json-nlohmann
)
