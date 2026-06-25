import qbs 1.0

QtcTool {
    name: "QtProfiler"

    Depends { name: "app_version_header" }
    Depends { name: "Utils" }
    Depends { name: "Core" }
    Depends { name: "Profiler" }
    Depends { name: "CommonTraceFormat" }
    Depends { name: "Tracing" }

    files: [
        "qtprofilerinit.cpp",
        "qtprofilerinit.h",
        "qtprofilermain.cpp",
        "qtprofilerrpc.cpp",
        "qtprofilerrpc.h",
        "qtprofilersettings.cpp",
        "qtprofilersettings.h",
        "qtprofilerwindow.cpp",
        "qtprofilerwindow.h",
        "mainsidebar.cpp",
        "mainsidebar.h",
        "recordingpage.cpp",
        "recordingpage.h",
        "welcomepage.cpp",
        "welcomepage.h",
        "schema/api.h",
    ]

    Group {
        name: "schema"
        Qt.core.resourcePrefix: "/qtprofiler/schema"
        files: [
            "schema/qtprofilerapi.json.schema",
        ]
        fileTags: "qt.core.resource_data"
    }
}
