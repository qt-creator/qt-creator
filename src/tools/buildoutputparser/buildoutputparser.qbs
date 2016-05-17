import qbs

QtcTool {
    name: "buildoutputparser"
    Depends { name: "Qt"; submodules: ["core", "widgets"]; }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "QmakeProjectManager" }
    Depends { name: "Utils" }
    files: [
        "main.cpp",
        "outputprocessor.cpp", "outputprocessor.h",
    ]
    Properties {
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin")
        cpp.rpaths: base.concat(["$ORIGIN/../" + qtc.ide_plugin_path])
    }
    cpp.defines: base.concat(qbs.targetOS.contains("windows") || qtc.testsEnabled
                             ? ["HAS_MSVC_PARSER"] : [])
}
