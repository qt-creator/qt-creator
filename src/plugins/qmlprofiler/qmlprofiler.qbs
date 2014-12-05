import qbs 1.0

QtcPlugin {
    name: "QmlProfiler"

    Depends { name: "Qt"; submodules: ["widgets", "network"] }
    Depends { name: "Qt.quick"; condition: product.condition; }
    Depends { name: "Aggregation" }
    Depends { name: "QmlJS" }
    Depends { name: "QmlDebug" }
    Depends { name: "QtcSsh" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "AnalyzerBase" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "TextEditor" }

    Group {
        name: "General"
        files: [
            "abstractqmlprofilerrunner.h",
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
            "qmlprofilerengine.cpp", "qmlprofilerengine.h",
            "qmlprofilereventsmodelproxy.cpp", "qmlprofilereventsmodelproxy.h",
            "qmlprofilereventview.cpp", "qmlprofilereventview.h",
            "qmlprofilermodelmanager.cpp", "qmlprofilermodelmanager.h",
            "qmlprofilernotesmodel.cpp", "qmlprofilernotesmodel.h",
            "qmlprofilerplugin.cpp", "qmlprofilerplugin.h",
            "qmlprofilerruncontrolfactory.cpp", "qmlprofilerruncontrolfactory.h",
            "qmlprofilerstatemanager.cpp", "qmlprofilerstatemanager.h",
            "qmlprofilerstatewidget.cpp", "qmlprofilerstatewidget.h",
            "qmlprofilerrangemodel.cpp", "qmlprofilerrangemodel.h",
            "qmlprofilertimelinemodel.cpp", "qmlprofilertimelinemodel.h",
            "qmlprofilertimelinemodelfactory.cpp", "qmlprofilertimelinemodelfactory.h",
            "qmlprofilertool.cpp", "qmlprofilertool.h",
            "qmlprofilertracefile.cpp", "qmlprofilertracefile.h",
            "qmlprofilertraceview.cpp", "qmlprofilertraceview.h",
            "qmlprofilertreeview.cpp", "qmlprofilertreeview.h",
            "qmlprofilerviewmanager.cpp", "qmlprofilerviewmanager.h",
            "qv8profilerdatamodel.cpp", "qv8profilerdatamodel.h",
            "qv8profilereventview.h", "qv8profilereventview.cpp",
            "timelineitemsrenderpass.cpp", "timelineitemsrenderpass.h",
            "timelinemodel.cpp", "timelinemodel.h", "timelinemodel_p.h",
            "timelinemodelaggregator.cpp", "timelinemodelaggregator.h",
            "timelinenotesrenderpass.cpp", "timelinenotesrenderpass.h",
            "timelinerenderer.cpp", "timelinerenderer.h",
            "timelinerenderpass.cpp", "timelinerenderpass.h",
            "timelinerenderstate.cpp", "timelinerenderstate.h",
            "timelineselectionrenderpass.cpp", "timelineselectionrenderpass.h",
            "timelinezoomcontrol.cpp", "timelinezoomcontrol.h"
        ]
    }

    Group {
        name: "QML"
        prefix: "qml/"
        files: [
            "ButtonsBar.qml",
            "Detail.qml",
            "CategoryLabel.qml",
            "MainView.qml",
            "Overview.js",
            "Overview.qml",
            "RangeDetails.qml",
            "RangeMover.qml",
            "SelectionRange.qml",
            "SelectionRangeDetails.qml",
            "TimeDisplay.qml",
            "TimelineContent.qml",
            "TimelineLabels.qml",
            "TimeMarks.qml",

            "qmlprofiler.qrc",

            "bindingloops.frag",
            "bindingloops.vert",
            "notes.frag",
            "notes.vert",
            "timelineitems.frag",
            "timelineitems.vert"
        ]
    }
}
