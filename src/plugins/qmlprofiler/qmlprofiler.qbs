import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "QmlProfiler"

    Depends { name: "Qt"; submodules: ["widgets", "network", "script", "declarative"] }
    Depends { name: "Core" }
    Depends { name: "AnalyzerBase" }
    Depends { name: "QmlProjectManager" }
    Depends { name: "Qt4ProjectManager" }
    Depends { name: "RemoteLinux" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "TextEditor" }
    Depends { name: "QmlDebug" }
    Depends { name: "QmlJS" }
    Depends { name: "QmlJSTools" }
    Depends { name: "CPlusPlus" }

    Depends { name: "cpp" }
    cpp.includePaths: base.concat("canvas")

    files: [
        "abstractqmlprofilerrunner.h",
        "localqmlprofilerrunner.cpp",
        "localqmlprofilerrunner.h",
        "qmlprofiler_global.h",
        "qmlprofilerattachdialog.cpp",
        "qmlprofilerattachdialog.h",
        "qmlprofilerattachdialog.ui",
        "qmlprofilerclientmanager.cpp",
        "qmlprofilerclientmanager.h",
        "qmlprofilerconstants.h",
        "qmlprofilerdatamodel.cpp",
        "qmlprofilerdatamodel.h",
        "qmlprofilerdetailsrewriter.cpp",
        "qmlprofilerdetailsrewriter.h",
        "qmlprofilerengine.cpp",
        "qmlprofilerengine.h",
        "qmlprofilereventview.cpp",
        "qmlprofilereventview.h",
        "qmlprofilerplugin.cpp",
        "qmlprofilerplugin.h",
        "qmlprofilerstatemanager.cpp",
        "qmlprofilerstatemanager.h",
        "qmlprofilerstatewidget.cpp",
        "qmlprofilerstatewidget.h",
        "qmlprofilertool.cpp",
        "qmlprofilertool.h",
        "qmlprofilertraceview.cpp",
        "qmlprofilertraceview.h",
        "qmlprofilerviewmanager.cpp",
        "qmlprofilerviewmanager.h",
        "qv8profilerdatamodel.cpp",
        "qv8profilerdatamodel.h",
        "remotelinuxqmlprofilerrunner.cpp",
        "remotelinuxqmlprofilerrunner.h",
        "timelinerenderer.cpp",
        "timelinerenderer.h",
        "canvas/qdeclarativecanvas.cpp",
        "canvas/qdeclarativecanvas_p.h",
        "canvas/qdeclarativecanvastimer.cpp",
        "canvas/qdeclarativecanvastimer_p.h",
        "canvas/qdeclarativecontext2d.cpp",
        "canvas/qdeclarativecontext2d_p.h",
        "canvas/qmlprofilercanvas.cpp",
        "canvas/qmlprofilercanvas.h",
        "qml/Detail.qml",
        "qml/Label.qml",
        "qml/MainView.qml",
        "qml/Overview.js",
        "qml/Overview.qml",
        "qml/RangeDetails.qml",
        "qml/RangeMover.qml",
        "qml/SelectionRange.qml",
        "qml/SelectionRangeDetails.qml",
        "qml/StatusDisplay.qml",
        "qml/TimeDisplay.qml",
        "qml/TimeMarks.qml",
        "qml/qmlprofiler.qrc",
    ]
}
