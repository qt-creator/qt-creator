import qbs 1.0
import qbs.Environment

QtcTool {
    name: "QtProfiler"
    windowsFileDescription: "Qt Profiler"
    windowsResourceFile: sourceDirectory + "/qtprofiler.rc"

    Depends { name: "codesign" }

    Properties {
        condition: qbs.targetOS.contains("macos")
        // The call-stack sampler needs the debugger entitlement here just as
        // much as in Qt Creator itself, and refuses to offer itself without it
        // (see src/app/app.qbs).
        codesign.enableCodeSigning: true
        codesign.signingType: "ad-hoc"
    }

    Group {
        name: "entitlements"
        condition: qbs.targetOS.contains("macos")
        files: ["../../../dist/installer/mac/Qt Profiler.entitlements"]
    }

    Depends { name: "app_version_header" }
    Depends { name: "Utils" }
    Depends { name: "Core" }
    Depends { name: "Profiler" }
    Depends { name: "CommonTraceFormat" }
    Depends { name: "Tracing"; required: false }
    // Usage statistics are only built in when Qt Insight Tracker is part of the
    // Qt in use and a server and token are given in the environment.
    Depends { name: "Qt.insighttracker"; required: false }
    // The tracker takes a host name; it refuses a server that starts with "http".
    property string insightUrl: (Environment.getEnv("QTPROFILER_INSIGHT_URL") || "")
                                    .replace(/^https?:\/\//, "").replace(/\/+$/, "")
    property string insightToken: Environment.getEnv("QTPROFILER_INSIGHT_TOKEN") || ""
    property bool withInsight: Qt.insighttracker.present && insightUrl && insightToken
    Properties {
        condition: withInsight
        cpp.defines: base.concat([
            "QTPROFILER_WITH_INSIGHT",
            'QTPROFILER_INSIGHT_URL="' + insightUrl + '"',
            'QTPROFILER_INSIGHT_TOKEN="' + insightToken + '"',
        ])
    }

    Group {
        name: "insight configuration"
        condition: withInsight
        Qt.core.resourcePrefix: "/"
        files: ["qtinsight.conf"]
        fileTags: "qt.core.resource_data"
    }

    condition: Tracing.present

    files: [
        "qtprofilerinit.cpp",
        "qtprofilerinit.h",
        "qtprofilerinsight.cpp",
        "qtprofilerinsight.h",
        "qtprofilermain.cpp",
        "qtprofilerrpc.cpp",
        "qtprofilerrpc.h",
        "qtprofilersettings.cpp",
        "qtprofilersettings.h",
        "qtprofilersidebar.cpp",
        "qtprofilersidebar.h",
        "qtprofilerwindow.cpp",
        "qtprofilerwindow.h",
        "schema/api.h",
    ]

    Group {
        name: "icons"
        Qt.core.resourcePrefix: "/qtprofiler/icon"
        Qt.core.resourceSourceBase: sourceDirectory + "/icon"
        files: [
            "icon/qtprofiler-16.png",
            "icon/qtprofiler-32.png",
            "icon/qtprofiler-128.png",
        ]
        fileTags: "qt.core.resource_data"
    }

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
        ]
        fileTags: "qt.core.resource_data"
    }
}
