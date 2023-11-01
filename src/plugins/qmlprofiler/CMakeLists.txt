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
    tests/qmlevent_test.cpp tests/qmlevent_test.h
    tests/qmleventlocation_test.cpp tests/qmleventlocation_test.h
    tests/qmleventtype_test.cpp tests/qmleventtype_test.h
    tests/qmlnote_test.cpp tests/qmlnote_test.h
    tests/qmlprofileranimationsmodel_test.cpp tests/qmlprofileranimationsmodel_test.h
    tests/qmlprofilerattachdialog_test.cpp tests/qmlprofilerattachdialog_test.h
    tests/qmlprofilerbindingloopsrenderpass_test.cpp tests/qmlprofilerbindingloopsrenderpass_test.h
    tests/qmlprofilerclientmanager_test.cpp tests/qmlprofilerclientmanager_test.h
    tests/qmlprofilerdetailsrewriter_test.cpp tests/qmlprofilerdetailsrewriter_test.h
    tests/qmlprofilertool_test.cpp tests/qmlprofilertool_test.h
    tests/qmlprofilertraceclient_test.cpp tests/qmlprofilertraceclient_test.h
    tests/qmlprofilertraceview_test.cpp tests/qmlprofilertraceview_test.h
    tests/tests.qrc
  )
else()
  set(TEST_SOURCES "")
endif()

set(QMLPROFILER_CPP_SOURCES
  debugmessagesmodel.cpp debugmessagesmodel.h
  flamegraphmodel.cpp flamegraphmodel.h
  flamegraphview.cpp flamegraphview.h
  inputeventsmodel.cpp inputeventsmodel.h
  memoryusagemodel.cpp memoryusagemodel.h
  pixmapcachemodel.cpp pixmapcachemodel.h
  qmlevent.cpp qmlevent.h
  qmleventlocation.cpp qmleventlocation.h
  qmleventtype.cpp qmleventtype.h
  qmlnote.cpp qmlnote.h
  qmlprofiler_global.h
  qmlprofilertr.h
  qmlprofileranimationsmodel.cpp qmlprofileranimationsmodel.h
  qmlprofilerattachdialog.cpp qmlprofilerattachdialog.h
  qmlprofilerbindingloopsrenderpass.cpp qmlprofilerbindingloopsrenderpass.h
  qmlprofilerclientmanager.cpp qmlprofilerclientmanager.h
  qmlprofilerconstants.h
  qmlprofilerdetailsrewriter.cpp qmlprofilerdetailsrewriter.h
  qmlprofilereventsview.h
  qmlprofilereventtypes.h
  qmlprofilermodelmanager.cpp qmlprofilermodelmanager.h
  qmlprofilernotesmodel.cpp qmlprofilernotesmodel.h
  qmlprofilerplugin.cpp qmlprofilerplugin.h
  qmlprofilerrangemodel.cpp qmlprofilerrangemodel.h
  qmlprofilerrunconfigurationaspect.cpp qmlprofilerrunconfigurationaspect.h
  qmlprofilerruncontrol.cpp qmlprofilerruncontrol.h
  qmlprofilersettings.cpp qmlprofilersettings.h
  qmlprofilerstatemanager.cpp qmlprofilerstatemanager.h
  qmlprofilerstatewidget.cpp qmlprofilerstatewidget.h
  qmlprofilerstatisticsmodel.cpp qmlprofilerstatisticsmodel.h
  qmlprofilerstatisticsview.cpp qmlprofilerstatisticsview.h
  qmlprofilertextmark.cpp qmlprofilertextmark.h
  qmlprofilertimelinemodel.cpp qmlprofilertimelinemodel.h
  qmlprofilertool.cpp qmlprofilertool.h
  qmlprofilertraceclient.cpp qmlprofilertraceclient.h
  qmlprofilertracefile.cpp qmlprofilertracefile.h
  qmlprofilertraceview.cpp qmlprofilertraceview.h
  qmlprofilerviewmanager.cpp qmlprofilerviewmanager.h
  qmltypedevent.cpp qmltypedevent.h
  scenegraphtimelinemodel.cpp scenegraphtimelinemodel.h
  quick3dmodel.cpp quick3dmodel.h
  quick3dframeview.cpp quick3dframeview.h
  quick3dframemodel.cpp quick3dframemodel.h
)

find_package(Qt6 COMPONENTS ShaderTools QUIET)

add_qtc_plugin(QmlProfiler
  CONDITION TARGET Tracing AND TARGET Qt6::ShaderTools
  DEPENDS QmlDebug QmlJS Tracing Qt::QuickWidgets
  PLUGIN_DEPENDS Core Debugger ProjectExplorer QtSupport TextEditor
  SOURCES
    ${TEST_SOURCES}
)

if (NOT TARGET QmlProfiler)
  return()
endif()

set(QMLPROFILER_QML_FILES
  qml/QmlProfilerFlameGraphView.qml
)

foreach(file IN LISTS QMLPROFILER_QML_FILES)
  get_filename_component(fileName "${file}" NAME)
  set_source_files_properties("${file}" PROPERTIES QT_RESOURCE_ALIAS "${fileName}")
endforeach()

qt_add_shaders(QmlProfiler "res_qmlprofilershaders"
  BATCHABLE
  PREFIX
    "/qt/qml/QtCreator/QmlProfiler"
  BASE
    "qml"
  FILES
    qml/bindingloops_qt6.frag
    qml/bindingloops_qt6.vert
)

qt_add_qml_module(QmlProfiler
  URI "QtCreator.QmlProfiler"
  VERSION "1.0"
  NO_PLUGIN
  RESOURCE_PREFIX "/qt/qml"
  QML_FILES
    ${QMLPROFILER_QML_FILES}
  RESOURCES
    ${QMLPROFILER_QML_RESOURCES}
  SOURCES
    ${QMLPROFILER_CPP_SOURCES}
)
