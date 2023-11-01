import qbs 1.0

QtcPlugin {
    name: "ScreenRecorder"

    Depends { name: "Core" }
    Depends { name: "Spinner" }

    files: [
        "cropandtrim.cpp",
        "cropandtrim.h",
        "export.cpp",
        "export.h",
        "ffmpegutils.cpp",
        "ffmpegutils.h",
        "record.cpp",
        "record.h",
        "screenrecorder.qrc",
        "screenrecorderconstants.h",
        "screenrecorderplugin.cpp",
        "screenrecordersettings.cpp",
        "screenrecordersettings.h",
    ]

    QtcTestFiles {
        files: [
            "screenrecorder_test.h",
            "screenrecorder_test.cpp",
        ]
    }
}
