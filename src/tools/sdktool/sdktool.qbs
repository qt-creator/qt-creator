import qbs.base 1.0
import "../QtcTool.qbs" as QtcTool

QtcTool {
    name: "sdktool"

    Depends { name: "Qt.core" }
    Depends { name: "Utils" }
    Depends { name: "app_version_header" }

    cpp.includePaths: "../../libs"
    cpp.defines: base.concat([qbs.targetOS.contains("osx")
            ? 'DATA_PATH="."' : 'DATA_PATH="../share/qtcreator"'])

    files: [
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
        "main.cpp",
        "operation.cpp",
        "operation.h",
        "rmkeysoperation.cpp",
        "rmkeysoperation.h",
        "rmkitoperation.cpp",
        "rmkitoperation.h",
        "rmqtoperation.cpp",
        "rmqtoperation.h",
        "rmtoolchainoperation.cpp",
        "rmtoolchainoperation.h",
        "settings.cpp",
        "settings.h",
    ]
}
