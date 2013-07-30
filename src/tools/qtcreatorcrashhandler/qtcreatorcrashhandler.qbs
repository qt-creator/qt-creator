import qbs.base 1.0
import "../QtcTool.qbs" as QtcTool

QtcTool {
    name: "qtcreator_crash_handler"
    condition: qbs.targetOS.contains("linux") && qbs.buildVariant == "debug"

    cpp.includePaths: [
        buildDirectory,
        "../../libs"
    ]

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

    Group {
        name: "Utils Sources"
        prefix: "../../libs/utils/"
        files: [
            "checkablemessagebox.cpp", "checkablemessagebox.h",
            "environment.cpp", "environment.h"
        ]
    }
}
