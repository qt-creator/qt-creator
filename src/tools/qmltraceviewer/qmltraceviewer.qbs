import qbs 1.0

QtcTool {
    name: "QmlTraceViewer"

    Depends { name: "app_version_header" }
    Depends { name: "Utils" }
    Depends { name: "Core" }
    Depends { name: "QmlProfiler" }

    files: [
        "qmltraceviewerinit.cpp",
        "qmltraceviewerinit.h",
        "qmltraceviewermain.cpp",
        "qmltraceviewerrpc.cpp",
        "qmltraceviewerrpc.h",
        "qmltraceviewersettings.cpp",
        "qmltraceviewersettings.h",
        "qmltraceviewerwindow.cpp",
        "qmltraceviewerwindow.h",
        "schema/api.h",
    ]
}
