add_qtc_plugin(QmlProfiler
  DEPENDS QmlDebug QmlJS Tracing Qt5::QuickWidgets
  PLUGIN_DEPENDS Core Debugger ProjectExplorer QtSupport TextEditor
  SOURCES
    debugmessagesmodel.cpp debugmessagesmodel.h
    flamegraphmodel.cpp flamegraphmodel.h
    flamegraphview.cpp flamegraphview.h
    inputeventsmodel.cpp inputeventsmodel.h
    memoryusagemodel.cpp memoryusagemodel.h
    pixmapcachemodel.cpp pixmapcachemodel.h
    qml/qmlprofiler.qrc
    qmlevent.cpp qmlevent.h
    qmleventlocation.cpp qmleventlocation.h
    qmleventtype.cpp qmleventtype.h
    qmlnote.cpp qmlnote.h
    qmlprofiler_global.h
    qmlprofileractions.cpp qmlprofileractions.h
    qmlprofileranimationsmodel.cpp qmlprofileranimationsmodel.h
    qmlprofilerattachdialog.cpp qmlprofilerattachdialog.h qmlprofilerattachdialog.ui
    qmlprofilerbindingloopsrenderpass.cpp qmlprofilerbindingloopsrenderpass.h
    qmlprofilerclientmanager.cpp qmlprofilerclientmanager.h
    qmlprofilerconfigwidget.cpp qmlprofilerconfigwidget.h qmlprofilerconfigwidget.ui
    qmlprofilerconstants.h
    qmlprofilerdetailsrewriter.cpp qmlprofilerdetailsrewriter.h
    qmlprofilereventsview.h
    qmlprofilereventtypes.h
    qmlprofilermodelmanager.cpp qmlprofilermodelmanager.h
    qmlprofilernotesmodel.cpp qmlprofilernotesmodel.h
    qmlprofileroptionspage.cpp qmlprofileroptionspage.h
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
)

extend_qtc_plugin(QmlProfiler
  CONDITION WITH_TESTS
  SOURCES
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
    tests/qmlprofilerconfigwidget_test.cpp tests/qmlprofilerconfigwidget_test.h
    tests/qmlprofilerdetailsrewriter_test.cpp tests/qmlprofilerdetailsrewriter_test.h
    tests/qmlprofilertool_test.cpp tests/qmlprofilertool_test.h
    tests/qmlprofilertraceclient_test.cpp tests/qmlprofilertraceclient_test.h
    tests/qmlprofilertraceview_test.cpp tests/qmlprofilertraceview_test.h
    tests/tests.qrc
)
