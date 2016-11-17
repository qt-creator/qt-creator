import qbs 1.0

QtcTool {
    name: "sdktool"

    Depends { name: "Qt.core" }
    Depends { name: "Utils" }
    Depends { name: "app_version_header" }

    cpp.defines: base.concat([qbs.targetOS.contains("macos")
            ? 'DATA_PATH="."'
            : qbs.targetOS.contains("windows") ? 'DATA_PATH="../share/qtcreator"'
                                               : 'DATA_PATH="../../share/qtcreator"'])

    files: [
        "addcmakeoperation.cpp", "addcmakeoperation.h",
        "adddebuggeroperation.cpp", "adddebuggeroperation.h",
        "adddeviceoperation.cpp", "adddeviceoperation.h",
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
        "rmcmakeoperation.cpp", "rmcmakeoperation.h",
        "rmdebuggeroperation.cpp", "rmdebuggeroperation.h",
        "rmdeviceoperation.cpp", "rmdeviceoperation.h",
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
