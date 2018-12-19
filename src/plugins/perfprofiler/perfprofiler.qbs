import qbs

QtcPlugin {
    name: "PerfProfiler"

    Depends { name: "Debugger" }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "Tracing" }
    Depends { name: "Utils" }

    Depends {
        name: "Qt"
        submodules: ["network", "quick", "quickwidgets"]
    }

    files: [
        "perfconfigeventsmodel.cpp",
        "perfconfigeventsmodel.h",
        "perfconfigwidget.cpp",
        "perfconfigwidget.h",
        "perfconfigwidget.ui",
        "perfdatareader.cpp",
        "perfdatareader.h",
        "perfevent.h",
        "perfeventtype.h",
        "perfloaddialog.cpp",
        "perfloaddialog.h",
        "perfloaddialog.ui",
        "perfoptionspage.cpp",
        "perfoptionspage.h",
        "perfprofiler_global.h",
        "perfprofilerconstants.h",
        "perfprofilerplugin.cpp",
        "perfprofilerplugin.h",
        "perfprofilertracemanager.cpp",
        "perfprofilertracemanager.h",
        "perftimelinemodel.cpp",
        "perftimelinemodel.h",
        "perftimelinemodelmanager.cpp",
        "perftimelinemodelmanager.h",
        "perftimelineresourcesrenderpass.cpp",
        "perftimelineresourcesrenderpass.h",
        "perfprofilerflamegraphmodel.cpp",
        "perfprofilerflamegraphmodel.h",
        "perfprofilerflamegraphview.cpp",
        "perfprofilerflamegraphview.h",
        "perfprofilerruncontrol.cpp",
        "perfprofilerruncontrol.h",
        "perfprofilerstatisticsmodel.cpp",
        "perfprofilerstatisticsmodel.h",
        "perfprofilerstatisticsview.cpp",
        "perfprofilerstatisticsview.h",
        "perfprofilertool.cpp",
        "perfprofilertool.h",
        "perfprofilertracefile.cpp",
        "perfprofilertracefile.h",
        "perfprofilertraceview.cpp",
        "perfprofilertraceview.h",
        "perfresourcecounter.cpp",
        "perfresourcecounter.h",
        "perfrunconfigurationaspect.cpp",
        "perfrunconfigurationaspect.h",
        "perfsettings.cpp",
        "perfsettings.h",
        "perftracepointdialog.cpp",
        "perftracepointdialog.h",
        "perftracepointdialog.ui",
        "perfprofiler.qrc",
    ]

    Group {
        name: "Unit tests"
        condition: qtc.testsEnabled
        prefix: "tests/"
        files: [
            "perfprofilertracefile_test.cpp",
            "perfprofilertracefile_test.h",
            "perfresourcecounter_test.cpp",
            "perfresourcecounter_test.h",
            "tests.qrc",
        ]
    }
}
