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
  )
else()
  set(TEST_SOURCES "")
endif()

set(QMLPROFILER_CPP_SOURCES
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
  qmlprofiler_global.h
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

add_qtc_plugin(QmlProfiler
  DEPENDS QmlDebug QmlJS Tracing CommonTraceFormat
  INCLUDES ${PROJECT_SOURCE_DIR}/src
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport TextEditor
  SOURCES
    ${TEST_SOURCES}
    ${QMLPROFILER_CPP_SOURCES}
    ../../libs/3rdparty/json/json.hpp
  SBOM_ARGS
    USE_ATTRIBUTION_FILES
    CREATE_SBOM_FOR_EACH_ATTRIBUTION
    ATTRIBUTION_FILE_PATHS
      "${QtCreator_SOURCE_DIR}/qt_attributions.json"
    ATTRIBUTION_IDS
      json-nlohmann
)
