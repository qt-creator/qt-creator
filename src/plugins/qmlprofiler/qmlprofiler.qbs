import qbs 1.0

QtcPlugin {
    name: "QmlProfiler"

    Depends { name: "Qt"; submodules: ["widgets", "network", "quick", "quickwidgets"] }
    Depends { name: "QmlJS" }
    Depends { name: "QmlDebug" }
    Depends { name: "QtcSsh" }
    Depends { name: "Utils" }
    Depends { name: "Timeline" }

    Depends { name: "Core" }
    Depends { name: "AnalyzerBase" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "TextEditor" }

    Group {
        name: "General"
        files: [
            "localqmlprofilerrunner.cpp", "localqmlprofilerrunner.h",
            "qmlprofiler_global.h",
            "qmlprofileranimationsmodel.h", "qmlprofileranimationsmodel.cpp",
            "qmlprofilerattachdialog.cpp", "qmlprofilerattachdialog.h",
            "qmlprofilerbasemodel.cpp", "qmlprofilerbasemodel.h", "qmlprofilerbasemodel_p.h",
            "qmlprofilerbindingloopsrenderpass.cpp","qmlprofilerbindingloopsrenderpass.h",
            "qmlprofilerclientmanager.cpp", "qmlprofilerclientmanager.h",
            "qmlprofilerconstants.h",
            "qmlprofilerdatamodel.cpp", "qmlprofilerdatamodel.h",
            "qmlprofilerdetailsrewriter.cpp", "qmlprofilerdetailsrewriter.h",
            "qmlprofilereventsmodelproxy.cpp", "qmlprofilereventsmodelproxy.h",
            "qmlprofilereventview.cpp", "qmlprofilereventview.h",
            "qmlprofilermodelmanager.cpp", "qmlprofilermodelmanager.h",
            "qmlprofilernotesmodel.cpp", "qmlprofilernotesmodel.h",
            "qmlprofilerplugin.cpp", "qmlprofilerplugin.h",
            "qmlprofilerruncontrolfactory.cpp", "qmlprofilerruncontrolfactory.h",
            "qmlprofilerrangemodel.cpp", "qmlprofilerrangemodel.h",
            "qmlprofilerruncontrol.cpp", "qmlprofilerruncontrol.h",
            "qmlprofilerstatemanager.cpp", "qmlprofilerstatemanager.h",
            "qmlprofilerstatewidget.cpp", "qmlprofilerstatewidget.h",
            "qmlprofilertimelinemodel.cpp", "qmlprofilertimelinemodel.h",
            "qmlprofilertimelinemodelfactory.cpp", "qmlprofilertimelinemodelfactory.h",
            "qmlprofilertool.cpp", "qmlprofilertool.h",
            "qmlprofilertracefile.cpp", "qmlprofilertracefile.h",
            "qmlprofilertraceview.cpp", "qmlprofilertraceview.h",
            "qmlprofilerviewmanager.cpp", "qmlprofilerviewmanager.h",
        ]
    }

    Group {
        name: "QML"
        prefix: "qml/"
        files: ["qmlprofiler.qrc"]
    }
}
