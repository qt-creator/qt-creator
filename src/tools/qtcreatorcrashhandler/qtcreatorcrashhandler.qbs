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

    files: [
        "../../libs/utils/checkablemessagebox.cpp",
        "../../libs/utils/checkablemessagebox.h",
        "../../libs/utils/environment.cpp",
        "../../libs/utils/environment.h",
        "backtracecollector.cpp",
        "backtracecollector.h",
        "crashhandler.cpp",
        "crashhandler.h",
        "crashhandlerdialog.cpp",
        "crashhandlerdialog.h",
        "crashhandlerdialog.ui",
        "main.cpp",
        "utils.cpp",
        "utils.h"
    ]
}
