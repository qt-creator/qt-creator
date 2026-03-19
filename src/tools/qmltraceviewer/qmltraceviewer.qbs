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

    Group {
        name: "schema"
        Qt.core.resourcePrefix: "/qmltraceviewer/schema"
        files: [
            "schema/qmltraceviewerapi.json.schema",
        ]
        fileTags: "qt.core.resource_data"
    }
}
