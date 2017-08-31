import qbs 1.0

QtcTool {
    name: "sdktool"

    Depends { name: "Qt.core" }
    Depends { name: "app_version_header" }

    property string libsDir: path + "/../../libs"

    cpp.defines: base.concat([
        "UTILS_LIBRARY",
        qbs.targetOS.contains("macos")
            ? 'DATA_PATH="."'
            : qbs.targetOS.contains("windows") ? 'DATA_PATH="../share/qtcreator"'
                                               : 'DATA_PATH="../../share/qtcreator"'
    ])
    cpp.dynamicLibraries: {
        if (qbs.targetOS.contains("windows"))
            return ["user32", "shell32"]
    }
    Properties {
        condition: qbs.targetOS.contains("macos")
        cpp.frameworks: ["Foundation"]
    }
    cpp.includePaths: base.concat([libsDir])

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

    Group {
        name: "Utils"
        prefix: libsDir + "/utils/"
        files: [
            "fileutils.cpp", "fileutils.h",
            "hostosinfo.cpp", "hostosinfo.h",
            "persistentsettings.cpp", "persistentsettings.h",
            "qtcassert.cpp", "qtcassert.h",
            "savefile.cpp", "savefile.h"
        ]
    }
    Group {
        name: "Utils/macOS"
        condition: qbs.targetOS.contains("macos")
        prefix: libsDir + "/utils/"
        files: [
            "fileutils_mac.h",
            "fileutils_mac.mm",
        ]
    }
}
