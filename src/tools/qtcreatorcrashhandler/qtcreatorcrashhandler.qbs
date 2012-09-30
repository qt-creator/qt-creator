import qbs.base 1.0
import "../QtcTool.qbs" as QtcTool

QtcTool {
    name: "qtcreator_crash_handler"
    condition: qbs.targetOS == "linux" && qbs.buildVariant == "debug"

    cpp.includePaths: [
        buildDirectory
    ]

    Depends { name: "cpp" }
    Depends { name: "Qt.widgets" }
    Depends { name: "app_version_header" }

    files: [
        "backtracecollector.cpp",
        "backtracecollector.h",
        "crashhandler.cpp",
        "crashhandler.h",
        "crashhandlerdialog.cpp",
        "crashhandlerdialog.h",
        "crashhandlerdialog.ui",
        "main.cpp",
        "utils.cpp",
        "utils.h",
    ]
}
