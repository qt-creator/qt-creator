import qbs 1.0

QtcTool {
    name: "sdktool"

    Depends { name: "Qt.core" }
    Depends { name: "app_version_header" }
    Depends { name: "Qt.testlib"; condition: project.withAutotests }

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
        "addabiflavor.cpp", "addabiflavor.h",
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
        "addvalueoperation.cpp",
        "addvalueoperation.h",
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
            "commandline.cpp", "commandline.h",
            "environment.cpp", "environment.h",
            "filepath.cpp", "filepath.h",
            "fileutils.cpp", "fileutils.h",
            "hostosinfo.cpp", "hostosinfo.h",
            "macroexpander.cpp", "macroexpander.h",
            "namevaluedictionary.cpp", "namevaluedictionary.h",
            "namevalueitem.cpp", "namevalueitem.h",
            "persistentsettings.cpp", "persistentsettings.h",
            "porting.h",
            "qtcassert.cpp", "qtcassert.h",
            "savefile.cpp", "savefile.h",
            "stringutils.cpp"
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
