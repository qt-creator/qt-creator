import qbs 1.0

QtcPlugin {
    name: "QmlProfiler"

    Depends { name: "Qt"; submodules: ["widgets", "network"] }

    Depends { name: "CommonTraceFormat" }
    Depends { name: "QmlJS" }
    Depends { name: "QmlDebug" }
    Depends { name: "Utils" }
    Depends { name: "Tracing" }
    Depends { name: "QtTaskTree" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "TextEditor" }

    Group {
        name: "General"
        files: [
            "ctfloader.cpp", "ctfloader.h",
            "macsampler.cpp", "macsampler.h",
            "sampletrace.cpp", "sampletrace.h",
            "ctfplainviewmanager.cpp", "ctfplainviewmanager.h",
            "ctfstatisticsmodel.cpp", "ctfstatisticsmodel.h",
            "ctfstatisticsview.cpp", "ctfstatisticsview.h",
            "ctftimelinemodel.cpp", "ctftimelinemodel.h",
            "ctftracemanager.cpp", "ctftracemanager.h",
            "ctfvisualizerconstants.h",
            "ctfvisualizertool.cpp", "ctfvisualizertool.h",
            "debugmessagesmodel.cpp", "debugmessagesmodel.h",
            "flamegraphmodel.cpp", "flamegraphmodel.h",
            "flamegraphview.cpp", "flamegraphview.h",
            "inputeventsmodel.cpp", "inputeventsmodel.h",
            "memoryusagemodel.cpp", "memoryusagemodel.h",
            "pixmapcachemodel.cpp", "pixmapcachemodel.h",
            "qmlnote.cpp", "qmlnote.h",
            "profilertr.h",
            "qmlprofiler_global.h",
            "qmlprofileranimationsmodel.h", "qmlprofileranimationsmodel.cpp",
            "qmlprofilerattachdialog.cpp", "qmlprofilerattachdialog.h",
            "qmlprofilerclientmanager.cpp", "qmlprofilerclientmanager.h",
            "qmlprofilerconstants.h",
            "qmlprofilerdetailsrewriter.cpp", "qmlprofilerdetailsrewriter.h",
            "qmlprofilereventsview.h",
            "qmlprofilermodelmanager.cpp", "qmlprofilermodelmanager.h",
            "qmlprofilernotesmodel.cpp", "qmlprofilernotesmodel.h",
            "qmlprofilerplainviewmanager.cpp", "qmlprofilerplainviewmanager.h",
            "qmlprofilerplugin.cpp",
            "qmlprofilerrunconfigurationaspect.cpp", "qmlprofilerrunconfigurationaspect.h",
            "qmlprofilerrangemodel.cpp", "qmlprofilerrangemodel.h",
            "qmlprofilerruncontrol.cpp", "qmlprofilerruncontrol.h",
            "qmlprofilersettings.cpp", "qmlprofilersettings.h",
            "qmlprofilerstatemanager.cpp", "qmlprofilerstatemanager.h",
            "qmlprofilerstatewidget.cpp", "qmlprofilerstatewidget.h",
            "qmlprofilerstatisticsmodel.cpp", "qmlprofilerstatisticsmodel.h",
            "qmlprofilerstatisticsview.cpp", "qmlprofilerstatisticsview.h",
            "qmlprofilertimelinemodel.cpp", "qmlprofilertimelinemodel.h",
            "qmlprofilertool.cpp", "qmlprofilertool.h",
            "qmlprofilertracefile.cpp", "qmlprofilertracefile.h",
            "qmlprofilertraceview.cpp", "qmlprofilertraceview.h",
            "quick3dmodel.cpp", "quick3dmodel.h",
            "quick3dframeview.cpp", "quick3dframeview.h",
            "quick3dframemodel.cpp", "quick3dframemodel.h",
            "scenegraphtimelinemodel.cpp", "scenegraphtimelinemodel.h",
        ]
    }

    QtcTestFiles {
        prefix: "tests/"
        files: [
            "debugmessagesmodel_test.cpp", "debugmessagesmodel_test.h",
            "fakedebugserver.cpp", "fakedebugserver.h",
            "flamegraphmodel_test.cpp", "flamegraphmodel_test.h",
            "flamegraphview_test.cpp", "flamegraphview_test.h",
            "inputeventsmodel_test.cpp", "inputeventsmodel_test.h",
            "localqmlprofilerrunner_test.cpp", "localqmlprofilerrunner_test.h",
            "memoryusagemodel_test.cpp", "memoryusagemodel_test.h",
            "pixmapcachemodel_test.cpp", "pixmapcachemodel_test.h",
            "qmlnote_test.cpp", "qmlnote_test.h",
            "qmlprofileranimationsmodel_test.cpp", "qmlprofileranimationsmodel_test.h",
            "qmlprofilerattachdialog_test.cpp", "qmlprofilerattachdialog_test.h",
            "qmlprofilerclientmanager_test.cpp", "qmlprofilerclientmanager_test.h",
            "qmlprofilerdetailsrewriter_test.cpp", "qmlprofilerdetailsrewriter_test.h",
            "qmlprofilertool_test.cpp", "qmlprofilertool_test.h",
            "qmlprofilertraceview_test.cpp", "qmlprofilertraceview_test.h",

            "tests.qrc"
        ]
    }
}
