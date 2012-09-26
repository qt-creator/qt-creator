import qbs.base 1.0
import "../QtcTool.qbs" as QtcTool

QtcTool {
    name: "sdktool"

    cpp.includePaths: [buildDirectory]
    cpp.defines: base.concat([qbs.targetOS === "mac"
            ? 'DATA_PATH="."' : 'DATA_PATH="../share/qtcreator"'])

    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
    Depends { name: "Utils" }
    Depends { name: "app_version_header" }

    files: [
        "main.cpp",
        "addkeysoperation.cpp",
        "addkeysoperation.h",
        "addkitoperation.cpp",
        "addkitoperation.h",
        "addqtoperation.cpp",
        "addqtoperation.h",
        "addtoolchainoperation.cpp",
        "addtoolchainoperation.h",
        "findkeyoperation.cpp",
        "findkeyoperation.h",
        "findvalueoperation.cpp",
        "findvalueoperation.h",
        "getoperation.cpp",
        "getoperation.h",
        "operation.h",
        "operation.cpp",
        "rmkeysoperation.cpp",
        "rmkeysoperation.h",
        "rmkitoperation.cpp",
        "rmkitoperation.h",
        "rmqtoperation.cpp",
        "rmqtoperation.h",
        "rmtoolchainoperation.cpp",
        "rmtoolchainoperation.h",
        "settings.cpp",
        "settings.h"
    ]
}
