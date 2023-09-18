QtcLibrary {
    name: "sdktoolLib"

    type: "staticlibrary"

    Depends { name: "Qt.core" }
    Depends { name: "app_version_header" }
    Depends { name: "Qt.testlib"; condition: qtc.withPluginTests }

    property string libsDir: path + "/../../libs"

    cpp.defines: {
        var defines = base;
        base.push(
            "UTILS_STATIC_LIBRARY",
            qbs.targetOS.contains("macos")
                ? 'DATA_PATH="."'
                : qbs.targetOS.contains("windows") ? 'DATA_PATH="../share/qtcreator"'
                                                   : 'DATA_PATH="../../share/qtcreator"');
        if (qtc.withPluginTests)
            defines.push("WITH_TESTS");
        return defines;
    }
    cpp.dynamicLibraries: {
        var libs = [];
        if (qbs.targetOS.contains("windows"))
            libs.push("user32", "shell32");
        if (qbs.toolchain.contains("msvc"))
            libs.push("dbghelp");
        return libs;
    }
    Properties {
        condition: qbs.targetOS.contains("macos")
        cpp.frameworks: ["Foundation"]
    }
    cpp.includePaths: base.concat([libsDir])

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [exportingProduct.sourceDirectory, exportingProduct.libsDir]
    }

    files: [
        "addabiflavor.cpp",
        "addabiflavor.h",
        "addcmakeoperation.cpp",
        "addcmakeoperation.h",
        "adddebuggeroperation.cpp",
        "adddebuggeroperation.h",
        "adddeviceoperation.cpp",
        "adddeviceoperation.h",
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
        "operation.cpp",
        "operation.h",
        "rmcmakeoperation.cpp",
        "rmcmakeoperation.h",
        "rmdebuggeroperation.cpp",
        "rmdebuggeroperation.h",
        "rmdeviceoperation.cpp",
        "rmdeviceoperation.h",
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
        "sdkpersistentsettings.cpp",
        "sdkpersistentsettings.h",
    ]
}
