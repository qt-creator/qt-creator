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
    cpp.rpaths: base.concat(qbs.targetOS.contains("osx")
            ? ["@executable_path/../"]
            : ["$ORIGIN/../" + project.ide_plugin_path])
    cpp.defines: base.concat(qbs.targetOS.contains("windows") || project.testsEnabled
                             ? ["HAS_MSVC_PARSER"] : [])
}
