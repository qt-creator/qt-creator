import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "QmlProfiler"

    Depends { name: "qt"; submodules: ['gui', 'network', 'script', 'declarative'] }
    Depends { name: "utils" }
    Depends { name: "extensionsystem" }
    Depends { name: "aggregation" }
    Depends { name: "Core" }
    Depends { name: "AnalyzerBase" }
    Depends { name: "QmlProjectManager" }
    Depends { name: "Qt4ProjectManager" }
    Depends { name: "RemoteLinux" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "symbianutils" }
    Depends { name: "TextEditor" }
    Depends { name: "QmlJSDebugClient" }
    Depends { name: "QmlJS" }
    Depends { name: "QmlJSTools" }
    Depends { name: "CPlusPlus" }

    Depends { name: "cpp" }
    cpp.includePaths: [
        "canvas",
        "../../shared/symbianutils",
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "abstractqmlprofilerrunner.h",
        "codaqmlprofilerrunner.cpp",
        "codaqmlprofilerrunner.h",
        "localqmlprofilerrunner.cpp",
        "localqmlprofilerrunner.h",
        "qmlprofiler_global.h",
        "qmlprofilerattachdialog.cpp",
        "qmlprofilerattachdialog.h",
        "qmlprofilerattachdialog.ui",
        "qmlprofilerconstants.h",
        "qmlprofilerdetailsrewriter.cpp",
        "qmlprofilerdetailsrewriter.h",
        "qmlprofilerengine.cpp",
        "qmlprofilerengine.h",
        "qmlprofilereventview.cpp",
        "qmlprofilereventview.h",
        "qmlprofilerplugin.cpp",
        "qmlprofilerplugin.h",
        "qmlprofilertool.cpp",
        "qmlprofilertool.h",
        "remotelinuxqmlprofilerrunner.cpp",
        "remotelinuxqmlprofilerrunner.h",
        "timelineview.cpp",
        "timelineview.h",
        "tracewindow.cpp",
        "tracewindow.h",
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
        "qml/Overview.qml",
        "qml/RangeDetails.qml",
        "qml/RangeMover.qml",
        "qml/SelectionRange.qml",
        "qml/SelectionRangeDetails.qml",
        "qml/StatusDisplay.qml",
        "qml/TimeDisplay.qml",
        "qml/TimeMarks.qml",
        "qml/qmlprofiler.qrc"
    ]
}

