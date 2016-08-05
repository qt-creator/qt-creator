import qbs 1.0

QtcPlugin {
    name: "QmlProfiler"

    Depends { name: "Qt"; submodules: ["widgets", "network", "quick", "quickwidgets"] }

    Depends { name: "FlameGraph" }
    Depends { name: "QmlJS" }
    Depends { name: "QmlDebug" }
    Depends { name: "Utils" }
    Depends { name: "Timeline" }

    Depends { name: "Core" }
    Depends { name: "Debugger" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "TextEditor" }

    Group {
        name: "General"
        files: [
            "debugmessagesmodel.cpp", "debugmessagesmodel.h",
            "flamegraphmodel.cpp", "flamegraphmodel.h",
            "flamegraphview.cpp", "flamegraphview.h",
            "inputeventsmodel.cpp", "inputeventsmodel.h",
            "localqmlprofilerrunner.cpp", "localqmlprofilerrunner.h",
            "memoryusagemodel.cpp", "memoryusagemodel.h",
            "pixmapcachemodel.cpp", "pixmapcachemodel.h",
            "qmlevent.cpp", "qmlevent.h",
            "qmleventlocation.cpp", "qmleventlocation.h",
            "qmleventtype.cpp", "qmleventtype.h",
            "qmlnote.cpp", "qmlnote.h",
            "qmlprofiler_global.h",
            "qmlprofileranimationsmodel.h", "qmlprofileranimationsmodel.cpp",
            "qmlprofilerattachdialog.cpp", "qmlprofilerattachdialog.h",
            "qmlprofilerbindingloopsrenderpass.cpp","qmlprofilerbindingloopsrenderpass.h",
            "qmlprofilerclientmanager.cpp", "qmlprofilerclientmanager.h",
            "qmlprofilerconfigwidget.cpp", "qmlprofilerconfigwidget.h",
            "qmlprofilerconfigwidget.ui", "qmlprofilerconstants.h",
            "qmlprofilerdatamodel.cpp", "qmlprofilerdatamodel.h",
            "qmlprofilerdetailsrewriter.cpp", "qmlprofilerdetailsrewriter.h",
            "qmlprofilereventsview.h",
            "qmlprofilereventtypes.h",
            "qmlprofilermodelmanager.cpp", "qmlprofilermodelmanager.h",
            "qmlprofilernotesmodel.cpp", "qmlprofilernotesmodel.h",
            "qmlprofileroptionspage.cpp", "qmlprofileroptionspage.h",
            "qmlprofilerplugin.cpp", "qmlprofilerplugin.h",
            "qmlprofilerrunconfigurationaspect.cpp", "qmlprofilerrunconfigurationaspect.h",
            "qmlprofilerruncontrolfactory.cpp", "qmlprofilerruncontrolfactory.h",
            "qmlprofilerrangemodel.cpp", "qmlprofilerrangemodel.h",
            "qmlprofilerruncontrol.cpp", "qmlprofilerruncontrol.h",
            "qmlprofilersettings.cpp", "qmlprofilersettings.h",
            "qmlprofilerstatemanager.cpp", "qmlprofilerstatemanager.h",
            "qmlprofilerstatewidget.cpp", "qmlprofilerstatewidget.h",
            "qmlprofilerstatisticsmodel.cpp", "qmlprofilerstatisticsmodel.h",
            "qmlprofilerstatisticsview.cpp", "qmlprofilerstatisticsview.h",
            "qmlprofilertimelinemodel.cpp", "qmlprofilertimelinemodel.h",
            "qmlprofilertool.cpp", "qmlprofilertool.h",
            "qmlprofilertraceclient.cpp", "qmlprofilertraceclient.h",
            "qmlprofilertracefile.cpp", "qmlprofilertracefile.h",
            "qmlprofilertraceview.cpp", "qmlprofilertraceview.h",
            "qmlprofilerviewmanager.cpp", "qmlprofilerviewmanager.h",
            "qmltypedevent.cpp", "qmltypedevent.h",
            "scenegraphtimelinemodel.cpp", "scenegraphtimelinemodel.h",
        ]
    }

    Group {
        name: "QML"
        prefix: "qml/"
        files: ["qmlprofiler.qrc"]
    }

    Group {
        name: "Unit tests"
        condition: qtc.testsEnabled
        prefix: "tests/"
        files: [
            "debugmessagesmodel_test.cpp", "debugmessagesmodel_test.h",
            "flamegraphmodel_test.cpp", "flamegraphmodel_test.h",
            "flamegraphview_test.cpp", "flamegraphview_test.h",
            "inputeventsmodel_test.cpp", "inputeventsmodel_test.h",
            "localqmlprofilerrunner_test.cpp", "localqmlprofilerrunner_test.h",
            "memoryusagemodel_test.cpp", "memoryusagemodel_test.h",
            "pixmapcachemodel_test.cpp", "pixmapcachemodel_test.h",
            "qmlevent_test.cpp", "qmlevent_test.h",
            "qmleventlocation_test.cpp", "qmleventlocation_test.h",
            "qmleventtype_test.cpp", "qmleventtype_test.h",
            "qmlnote_test.cpp", "qmlnote_test.h",
            "qmlprofileranimationsmodel_test.cpp", "qmlprofileranimationsmodel_test.h",
            "qmlprofilerattachdialog_test.cpp", "qmlprofilerattachdialog_test.h",
            "qmlprofilerbindingloopsrenderpass_test.cpp",
            "qmlprofilerbindingloopsrenderpass_test.h",
            "qmlprofilerclientmanager_test.cpp", "qmlprofilerclientmanager_test.h",
            "qmlprofilerconfigwidget_test.cpp", "qmlprofilerconfigwidget_test.h",
        ]
    }
}
