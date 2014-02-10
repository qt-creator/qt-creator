import qbs.base 1.0

import QtcPlugin

QtcPlugin {
    name: "QmlProfiler"
    minimumQtVersion: "5.1"

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
            "abstracttimelinemodel.h", "abstracttimelinemodel.cpp",
            "localqmlprofilerrunner.cpp", "localqmlprofilerrunner.h",
            "qmlprofiler_global.h",
            "qmlprofilerattachdialog.cpp", "qmlprofilerattachdialog.h",
            "qmlprofilerclientmanager.cpp", "qmlprofilerclientmanager.h",
            "qmlprofilerconstants.h",
            "qmlprofilerdetailsrewriter.cpp", "qmlprofilerdetailsrewriter.h",
            "qmlprofilerengine.cpp", "qmlprofilerengine.h",
            "qmlprofilereventsmodelproxy.cpp", "qmlprofilereventsmodelproxy.h",
            "qmlprofilereventview.cpp", "qmlprofilereventview.h",
            "qmlprofilermodelmanager.cpp", "qmlprofilermodelmanager.h",
            "qmlprofilerpainteventsmodelproxy.h", "qmlprofilerpainteventsmodelproxy.cpp",
            "qmlprofilerplugin.cpp", "qmlprofilerplugin.h",
            "qmlprofilerprocessedmodel.cpp", "qmlprofilerprocessedmodel.h",
            "qmlprofilerruncontrolfactory.cpp", "qmlprofilerruncontrolfactory.h",
            "qmlprofilersimplemodel.cpp", "qmlprofilersimplemodel.h",
            "qmlprofilerstatemanager.cpp", "qmlprofilerstatemanager.h",
            "qmlprofilerstatewidget.cpp", "qmlprofilerstatewidget.h",
            "qmlprofilertimelinemodelproxy.cpp", "qmlprofilertimelinemodelproxy.h",
            "qmlprofilertool.cpp", "qmlprofilertool.h",
            "qmlprofilertracefile.cpp", "qmlprofilertracefile.h",
            "qmlprofilertraceview.cpp", "qmlprofilertraceview.h",
            "qmlprofilertreeview.cpp", "qmlprofilertreeview.h",
            "qmlprofilerviewmanager.cpp", "qmlprofilerviewmanager.h",
            "qv8profilerdatamodel.cpp", "qv8profilerdatamodel.h",
            "qv8profilereventview.h", "qv8profilereventview.cpp",
            "sortedtimelinemodel.h", "sortedtimelinemodel.cpp",
            "timelinemodelaggregator.cpp", "timelinemodelaggregator.h",
            "timelinerenderer.cpp", "timelinerenderer.h",
        ]
    }

    Group {
        name: "QML"
        prefix: "qml/"
        files: [
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
            "TimeMarks.qml",
            "HorizontalGradientBorder.qml",
            "VerticalGradientBorder.qml",
            "qmlprofiler.qrc",
        ]
    }
}
