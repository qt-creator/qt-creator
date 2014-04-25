import qbs 1.0
import QtcTool

QtcTool {
    name: "winrtdebughelper"
    condition: qbs.targetOS.contains("windows")

    Depends { name: "Qt.network" }

    files: [
        "winrtdebughelper.cpp",
    ]
}
