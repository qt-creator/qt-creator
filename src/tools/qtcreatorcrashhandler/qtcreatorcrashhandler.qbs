import qbs 1.0

QtcTool {
    name: "qtcreator_crash_handler"
    condition: qbs.targetOS.contains("linux") && qbs.buildVariant == "debug"

    Depends { name: "Utils" }
    Depends { name: "Qt.widgets" }
    Depends { name: "app_version_header" }

    Group {
        name: "Crash Handler Sources"
        files: [
            "backtracecollector.cpp", "backtracecollector.h",
            "crashhandler.cpp", "crashhandler.h",
            "crashhandlerdialog.cpp", "crashhandlerdialog.h", "crashhandlerdialog.ui",
            "main.cpp",
            "utils.cpp", "utils.h"
        ]
    }
}
