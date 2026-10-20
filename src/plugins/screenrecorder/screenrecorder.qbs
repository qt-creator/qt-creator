import qbs 1.0

QtcPlugin {
    name: "ScreenRecorder"

    property bool usePortal: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("macos")

    Depends { name: "Core" }
    Depends { name: "Spinner" }
    Depends { name: "Qt.dbus"; condition: usePortal }

    Properties {
        condition: usePortal
        cpp.defines: base.concat("QTC_SCREENRECORDER_WITH_PORTAL")
    }

    files: [
        "cropandtrim.cpp",
        "cropandtrim.h",
        "export.cpp",
        "export.h",
        "ffmpegutils.cpp",
        "ffmpegutils.h",
        "portalcapture.cpp",
        "portalcapture.h",
        "record.cpp",
        "record.h",
        "screenrecorderconstants.h",
        "screenrecorderplugin.cpp",
        "screenrecordersettings.cpp",
        "screenrecordersettings.h",
        "screenrecordertr.h",
    ]

    QtcTestFiles {
        files: [
            "screenrecorder_test.h",
            "screenrecorder_test.cpp",
        ]
    }
}
