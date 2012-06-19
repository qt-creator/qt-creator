import qbs.base 1.0
import "../QtcTool.qbs" as QtcTool

QtcTool {
    name: "qtcdebugger"
    condition: qbs.targetOS == "windows"

    cpp.includePaths: [
        buildDirectory,
        "../../shared/registryaccess"
    ]
    cpp.dynamicLibraries: [
        "psapi",
        "advapi32"
    ]

    Depends { name: "cpp" }
    Depends { name: "Qt.widgets" }
    Depends { name: "app_version_header" }

    files: [
        "main.cpp",
        "../../shared/registryaccess/registryaccess.cpp",
        "../../shared/registryaccess/registryaccess.h"
    ]
}
