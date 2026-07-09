import qbs 1.0

QtcTool {
    name: "QtProfiler"

    Depends { name: "app_version_header" }
    Depends { name: "Utils" }
    Depends { name: "Core" }
    Depends { name: "Profiler" }
    Depends { name: "CommonTraceFormat" }
    Depends { name: "Tracing"; required: false }

    condition: Tracing.present

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

    // Compile the editor themes (and every file they include) straight into the
    // binary, so the application loads them from ":/qtprofiler/themes" instead of
    // a themes directory on disk. The includes are resolved relative to the theme
    // file, so they must live next to it under the same resource prefix.
    Group {
        name: "themes"
        prefix: project.ide_source_tree + "/share/qtcreator/themes/"
        Qt.core.resourcePrefix: "/qtprofiler/themes"
        Qt.core.resourceSourceBase: project.ide_source_tree + "/share/qtcreator/themes"
        files: [
            "dark-2024.creatortheme",
            "light-2024.creatortheme",
            "dark.figmatokens",
            "light.figmatokens",
            "primitive-colors.inc",
            "2024.tokenmapping",
            "ds.tokenmapping",
            "qmldesigner-dark.inc",
            "qmldesigner-light.inc",
        ]
        fileTags: "qt.core.resource_data"
    }
}
